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
 * The main window's title names the profile the main window is showing, and a
 * profile in a detached window is not one of those. Detaching used to leave the
 * title naming whichever profile was activated last, and closing the profile left
 * behind only cleared it when no profile was loaded anywhere - so the window
 * kept the name of a profile that was no longer in it (PR #8195).
 *
 * Run with: ctest -R MainWindowTitleTest -V
 */

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TDetachedWindow.h"
#include "TTabBar.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MainWindowTitleTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mFirstProfile = qsl("MainWindowTitle-Test-First");
    const QString mSecondProfile = qsl("MainWindowTitle-Test-Second");
    QString mPort;
    const QString mLocalhost = qsl("localhost");

    void startProfile(const QString& profileName)
    {
        QDir(MudletApp::getMudletPath(enums::profileHomePath, profileName)).removeRecursively();
        Host* pHost = TestProfile::create(profileName, mLocalhost, mPort);
        if (!pHost) {
            QTest::qFail(qPrintable(qsl("no active host after creating '%1'").arg(profileName)), __FILE__, __LINE__);
            return;
        }
        QSignalSpy connected(&(pHost->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(2000)) {
            QTest::qFail(qPrintable(qsl("'%1' could not connect to the stub server").arg(profileName)), __FILE__, __LINE__);
        }
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

        startProfile(mFirstProfile);
        if (QTest::currentTestFailed()) {
            return;
        }
        startProfile(mSecondProfile);
    }

    // Nothing owns a detached window, so the reattach has to be what deletes it
    void cleanup()
    {
        if (!mudlet::self() || !mudlet::self()->getDetachedWindows().contains(mSecondProfile)) {
            return;
        }
        const QPointer<TDetachedWindow> detachedWindow = mudlet::self()->getDetachedWindows().value(mSecondProfile);
        mudlet::self()->slot_tabReattachRequested(mSecondProfile);
        // the reattach frees the window through deleteLater()
        QVERIFY(QTest::qWaitFor(
                [&detachedWindow]() {
                    return detachedWindow.isNull();
                },
                2000));
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

    // PR #8195: with one profile detached and the other closed, the main window
    // shows no profile at all - but a profile was still loaded, which is all the
    // old title reset looked at, so the window kept the closed profile's name.
    void test_closingTheOnlyProfileLeftInTheMainWindowClearsItsNameFromTheTitle()
    {
        const int secondTab = mudlet::self()->mpTabBar->tabIndex(mSecondProfile);
        QVERIFY2(secondTab >= 0, "the second profile has no tab to detach");
        mudlet::self()->slot_tabDetachRequested(secondTab, QPoint(200, 200));
        QVERIFY2(mudlet::self()->getDetachedWindows().contains(mSecondProfile), "the second profile did not detach");

        const int firstTab = mudlet::self()->mpTabBar->tabIndex(mFirstProfile);
        QVERIFY2(firstTab >= 0, "the first profile is not in the main window");
        QVERIFY2(mudlet::self()->windowTitle().startsWith(mFirstProfile), "the main window title does not name the profile it is left showing, so clearing it cannot be told from doing nothing");

        mudlet::self()->slot_closeProfileRequested(firstTab);
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return HostManager::self()->getHost(mFirstProfile) == nullptr;
                         },
                         10000),
                 "the first profile never closed");

        // the detached profile is still loaded, which is the whole point: the
        // old title reset only fired when nothing was loaded anywhere
        QVERIFY2(HostManager::self()->getHost(mSecondProfile), "the detached profile closed too, so an empty title proves nothing");
        QCOMPARE(mudlet::self()->windowTitle(), MudletApp::scmVersion());
    }
};

#include "MainWindowTitleTest.moc"
MUDLET_GROUPED_TEST_MAIN(MainWindowTitleTest)
