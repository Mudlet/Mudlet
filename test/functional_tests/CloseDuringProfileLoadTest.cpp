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
 * A close delivered from inside a profile load's event-loop pump (the close
 * button, a desktop shutdown) must be held until the load returns, then happen
 * on its own: accepting it there deletes every Host underneath the load still
 * using one of them.
 */

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "mudlet.h"

#include "GroupedTest.h"

#include <QPointer>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest/QtTest>

class CloseDuringProfileLoadTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    const QString mProfileName = qsl("CloseDuringProfileLoad-Test");
    const QString mSecondProfileName = qsl("CloseDuringProfileLoad-Test-2");
    QPointer<mudlet> mWindow;
    bool mCloseAskedFor = false;
    bool mCloseRefused = false;

    static bool provisionProfileOnDisk(const QString& name)
    {
        return QDir().mkpath(mudlet::getMudletPath(enums::profileHomePath, name)) && mudlet::self()->writeProfileData(name, qsl("url"), qsl("localhost")).first
               && mudlet::self()->writeProfileData(name, qsl("port"), qsl("23")).first;
    }

    // Fires from the first event-loop pump inside a load, as the window's
    // close button or a desktop shutdown would
    void closeFromTheNextPump()
    {
        mCloseAskedFor = false;
        mCloseRefused = false;
        QTimer::singleShot(0, mWindow, [this]() {
            mCloseAskedFor = true;
            // close() reports false only when its QCloseEvent was ignored
            mCloseRefused = !mWindow->close();
        });
    }

    void verifyTheCloseWasHeldAndThenHappened()
    {
        QVERIFY2(mCloseAskedFor, "the load never pumped the event loop, so this test asked for nothing");
        QVERIFY2(mWindow, "the main window was deleted underneath the profile load");
        QVERIFY2(mCloseRefused, "the close was accepted in the middle of the profile load");
        QPointer<Host> host = mWindow->getHostManager().getHost(mProfileName);
        QVERIFY2(host, "the profile was closed in the middle of its own load");

        // The held close happens on its own once the load returns; the window
        // is WA_DeleteOnClose, so it goes on a deferred delete once accepted
        QTRY_VERIFY2_WITH_TIMEOUT(mWindow.isNull(), "the close asked for during the load never happened", 30000);
        QVERIFY2(host.isNull(), "the window closed but left the profile loaded");
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

    // Each case lets the window close and delete itself, so each gets its own
    void init()
    {
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        // A settings file that already holds something is how
        // mudletUsedBefore() recognises an existing player, which keeps the
        // first-run UI tour and the starter UI package out of this test
        mudlet::getQSettings()->setValue(qsl("uiTourShown"), true);
        mudlet::getQSettings()->sync();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QVERIFY2(mudlet::self()->experiencedMudletPlayer(), "the first-run UI would open over this test");
        mWindow = mudlet::self();
    }

    // Already null when the case let the window close; a failed case leaves it
    void cleanup() { delete mudlet::self(); }

    void test_aCloseDuringAutoLoginWaitsForEveryProfileToLoad()
    {
        QVERIFY(provisionProfileOnDisk(mProfileName));
        QVERIFY(provisionProfileOnDisk(mSecondProfileName));
        closeFromTheNextPump();

        mWindow->startAutoLogin({mProfileName, mSecondProfileName}, true);

        QVERIFY2(mWindow, "the main window was deleted underneath the profile load");
        QVERIFY2(mWindow->getHostManager().getHost(mSecondProfileName), "the close cut the auto-login batch short");
        verifyTheCloseWasHeldAndThenHappened();
    }

    // The connection dialog loads the profile and then finishes the connection
    // itself, with no pump between the two
    void test_aCloseDuringAConnectionDialogLoadWaitsForTheLoad()
    {
        QVERIFY(provisionProfileOnDisk(mProfileName));
        QVERIFY(mWindow->loadProfile(mProfileName, false));
        closeFromTheNextPump();

        mWindow->slot_connectionDialogueFinished(mProfileName, false);

        verifyTheCloseWasHeldAndThenHappened();
    }
};

#include "CloseDuringProfileLoadTest.moc"
MUDLET_GROUPED_TEST_MAIN(CloseDuringProfileLoadTest)
