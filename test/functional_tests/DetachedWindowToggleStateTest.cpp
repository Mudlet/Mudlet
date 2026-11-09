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
#include <QMenu>
#include <QMenuBar>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
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

// A detached window's "Compact input line" and "Multiview" items toggle state held outside the window, so their
// checkmarks have to show that state when their menu opens, or clicking one does the opposite of what it says.
class DetachedWindowToggleStateTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mFirstHostname = qsl("DetachedWindowToggleState-First");
    const QString mSecondHostname = qsl("DetachedWindowToggleState-Second");

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
        // Two of them: slot_tabDetachRequested() refuses index 0
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

    void init()
    {
        mudlet::self()->slot_multiView(false);
        for (auto pHost : *HostManager::self()) {
            pHost->setCompactInputLine(false);
        }
        QVERIFY2(mudlet::self()->getDetachedWindows().isEmpty(), "a detached window was left over from an earlier case");
    }

    void cleanup()
    {
        const QStringList detachedProfiles = mudlet::self()->getDetachedWindows().keys();
        for (const QString& profileName : detachedProfiles) {
            mudlet::self()->slot_tabReattachRequested(profileName);
        }
    }

    void test_compactInputLineShowsTheProfilesSetting()
    {
        Host* pHost = HostManager::self()->getHost(mSecondHostname);
        QVERIFY(pHost);
        pHost->setCompactInputLine(true);
        TDetachedWindow* pWindow = detachProfile(mSecondHostname);
        QVERIFY(pWindow);
        QAction* pItem = pWindow->findChild<QAction*>(qsl("menuCompactInputLine"));
        QVERIFY(pItem);

        openMenu(pWindow, pItem);
        QVERIFY2(pItem->isChecked(), "the item is unchecked for a profile whose input line is compact");

        pItem->trigger();
        QVERIFY(!pHost->getCompactInputLine());
        openMenu(pWindow, pItem);
        QVERIFY(!pItem->isChecked());

        pHost->setCompactInputLine(true);
        openMenu(pWindow, pItem);
        QVERIFY2(pItem->isChecked(), "a change made outside the window did not reach its checkmark");
    }

    void test_multiViewShowsTheApplicationSetting()
    {
        TDetachedWindow* pWindow = detachProfile(mSecondHostname);
        QVERIFY(pWindow);
        QAction* pItem = pWindow->findChild<QAction*>(qsl("menuMultiView"));
        QVERIFY(pItem);

        mudlet::self()->slot_multiView(true);
        openMenu(pWindow, pItem);
        QVERIFY2(pItem->isChecked(), "the item is unchecked while multiview is on");

        pItem->trigger();
        QVERIFY(!mudlet::self()->multiView());
        openMenu(pWindow, pItem);
        QVERIFY(!pItem->isChecked());
    }

private:
    // Emits aboutToShow() for the menu holding pItem, as opening it would
    static void openMenu(TDetachedWindow* pWindow, QAction* pItem)
    {
        const QList<QMenu*> menus = pWindow->menuBar()->findChildren<QMenu*>();
        for (QMenu* pMenu : menus) {
            if (pMenu->actions().contains(pItem)) {
                emit pMenu->aboutToShow();
                return;
            }
        }
        QTest::qFail("no menu holds the item", __FILE__, __LINE__);
    }

    TDetachedWindow* detachProfile(const QString& profileName)
    {
        const int tabIndex = mudlet::self()->mpTabBar->tabIndex(profileName);
        if (tabIndex < 1) {
            QTest::qFail(qPrintable(qsl("'%1' is at tab %2, which slot_tabDetachRequested() will not detach").arg(profileName).arg(tabIndex)), __FILE__, __LINE__);
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

#include "DetachedWindowToggleStateTest.moc"
MUDLET_GROUPED_TEST_MAIN(DetachedWindowToggleStateTest)
