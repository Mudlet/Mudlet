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

/*
 * Every action in a Connect drop-down has an icon, so the menu does not show
 * one entry with a gap where the others have a picture. The main window and a
 * detached profile window each build their own copy of the drop-down.
 *
 * Run with: ctest -R ConnectButtonIconsTest -V
 */

#include <QAction>
#include <QDir>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QToolButton>
#include <QtTest/QtTest>

#include <memory>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TDetachedWindow.h"
#include "TTabBar.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class ConnectButtonIconsTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mFirstProfile = qsl("ConnectButtonIcons-Test-First");
    const QString mSecondProfile = qsl("ConnectButtonIcons-Test-Second");

    void startProfile(const QString& profileName)
    {
        QDir(MudletApp::getMudletPath(enums::profileHomePath, profileName)).removeRecursively();
        Host* pHost = TestProfile::create(profileName, mLocalhost, mPort);
        if (!pHost) {
            QTest::qFail(qPrintable(qsl("no active host after creating '%1'").arg(profileName)), __FILE__, __LINE__);
            return;
        }
        QSignalSpy connected(&(pHost->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(2s)) {
            QTest::qFail(qPrintable(qsl("'%1' could not connect to the stub server").arg(profileName)), __FILE__, __LINE__);
        }
    }

    static void verifyEveryActionHasAnIcon(QWidget* pWindow, const QString& windowName)
    {
        auto pButton = pWindow->findChild<QToolButton*>(qsl("connect"));
        QVERIFY2(pButton, qPrintable(qsl("the %1 has no Connect button").arg(windowName)));

        bool sawDisconnect = false;
        for (const auto pAction : pButton->actions()) {
            sawDisconnect |= pAction->objectName() == qsl("disconnect");
            QVERIFY2(!pAction->icon().pixmap(16).isNull(), qPrintable(qsl("the %1's Connect drop-down entry '%2' has no icon").arg(windowName, pAction->text())));
        }
        QVERIFY2(sawDisconnect, qPrintable(qsl("the %1's Connect drop-down has no Disconnect entry, so its icon went unchecked").arg(windowName)));
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
        QVERIFY2(mpServer->isListening(), qPrintable(qsl("TelnetServerStub failed to start: %1").arg(mpServer->errorString())));
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        // Two of them, because slot_tabDetachRequested() refuses index 0
        startProfile(mFirstProfile);
        if (QTest::currentTestFailed()) {
            return;
        }
        startProfile(mSecondProfile);
    }

    void cleanup()
    {
        if (mudlet::self() && mudlet::self()->getDetachedWindows().contains(mSecondProfile)) {
            mudlet::self()->slot_tabReattachRequested(mSecondProfile);
        }
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            const QString firstPath = MudletApp::getMudletPath(enums::profileHomePath, mFirstProfile);
            const QString secondPath = MudletApp::getMudletPath(enums::profileHomePath, mSecondProfile);
            delete mudlet::self();
            QDir(firstPath).removeRecursively();
            QDir(secondPath).removeRecursively();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_everyMainWindowConnectDropDownActionHasAnIcon() { verifyEveryActionHasAnIcon(mudlet::self(), qsl("main window")); }

    void test_everyDetachedWindowConnectDropDownActionHasAnIcon()
    {
        const int secondTab = mudlet::self()->mpTabBar->tabIndex(mSecondProfile);
        QVERIFY2(secondTab >= 0, "the second profile has no tab to detach");
        mudlet::self()->slot_tabDetachRequested(secondTab, QPoint(200, 200));
        TDetachedWindow* pDetachedWindow = mudlet::self()->getDetachedWindows().value(mSecondProfile);
        QVERIFY2(pDetachedWindow, "the second profile did not detach");

        verifyEveryActionHasAnIcon(pDetachedWindow, qsl("detached window"));
    }
};

#include "ConnectButtonIconsTest.moc"
MUDLET_GROUPED_TEST_MAIN(ConnectButtonIconsTest)
