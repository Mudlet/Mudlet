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
 * What the connection dialog does with the profile it was asked to open, driven
 * against a stub server on a loopback port:
 *
 *  - the dialog takes itself off screen before the load starts, rather than
 *    sitting there through it collecting further clicks (PR #5313, issue #5303);
 *  - asking again for the profile that is already open reconnects it instead of
 *    loading it a second time (PR #7778, issue #7698).
 *
 * Run with: ctest -R ConnectionDialogOpenProfileTest -V
 */

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "TelnetServerStub.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include <QtTest/QtTest>

#include <QLineEdit>
#include <QListWidget>
#include <QSignalSpy>
#include <chrono>

#include "GroupedTest.h"

using namespace std::chrono_literals;

class ConnectionDialogOpenProfileTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mXdgDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;

    const QString mProfileName = qsl("ConnDialogOpenProfile-Test");
    const QString mLocalhost = qsl("127.0.0.1");

    // written from a lambda on mudlet's own signal, so they outlive the case
    bool mDialogOnScreenDuringTheLoad = true;
    bool mDialogAliveDuringTheLoad = false;

    dlgConnectionProfiles* dialog() const { return mudlet::self()->mpConnectionDialog.data(); }

    // Each case here closes the dialog, which then deletes itself through the
    // event loop - asking for a new one before that has happened only raises
    // the dying one. One still on screen is the dialog this test started with,
    // which needs no reopening.
    bool reopenDialog() const
    {
        if (auto* pDialog = dialog(); pDialog && pDialog->isVisible()) {
            return true;
        }
        if (!QTest::qWaitFor(
                    []() {
                        return mudlet::self()->mpConnectionDialog.isNull();
                    },
                    5000)) {
            return false;
        }
        mudlet::self()->slot_showConnectionDialog();
        return QTest::qWaitFor(
                []() {
                    return !mudlet::self()->mpConnectionDialog.isNull() && mudlet::self()->mpConnectionDialog->isVisible();
                },
                5000);
    }

    // picks the test profile the way a click on it does. slot_itemClicked()
    // ignores a repeat of the profile it saw less than 100ms ago, and it keeps
    // that in function-local statics - so the debounce outlives the dialog
    // reopenDialog() throws away, and a click landing inside it would leave the
    // form as the rebuild blanked it
    bool selectTestProfile() const
    {
        QTest::qWait(300ms);
        auto* pDialog = dialog();
        if (!pDialog) {
            return false;
        }
        const auto items = pDialog->findData(*pDialog->listWidget_profiles, mProfileName, dlgConnectionProfiles::csmNameRole);
        if (items.isEmpty()) {
            return false;
        }
        pDialog->listWidget_profiles->setCurrentItem(items.first());
        pDialog->slot_itemClicked(items.first());
        return pDialog->profile_name_entry->text() == mProfileName;
    }

    // The case below reads what the dialog does about a profile that is
    // already open, and the case above it is what normally leaves one open.
    // The grouped runner also takes a single method name, so opening it here
    // is what lets that case run on its own.
    bool openTheTestProfile() const
    {
        if (auto* pHost = mudlet::self()->getActiveHost(); pHost && pHost->getName() == mProfileName) {
            return true;
        }
        if (!reopenDialog() || !selectTestProfile()) {
            return false;
        }
        dialog()->slot_load();
        return QTest::qWaitFor(
                [this]() {
                    auto* pHost = mudlet::self()->getActiveHost();
                    return pHost && pHost->getName() == mProfileName;
                },
                15000);
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - cannot redirect the config dir for this test");
        }

        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(mXdgDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mXdgDir.path()))); // profiles/ = XDG opt-in
        qputenv("XDG_CONFIG_HOME", mXdgDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        // an OS-assigned port, so parallel test runs cannot collide on a fixed one
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->serverPort() != 0, "TelnetServerStub failed to bind a loopback port");

        mudlet::start();
        mudlet::self()->setupConfig();
        QVERIFY(MudletApp::getMudletPath(enums::profilesPath).startsWith(mXdgDir.path()));
        // a settings file that already holds something is how a returning
        // player is recognised, which keeps the first-run invitation - it hides
        // the games list - out of this test. It has to be written before
        // init(), which stamps this config dir with a first-launch date of its
        // own that would read as a brand new install
        MudletApp::getQSettings()->setValue(qsl("uiTourShown"), true);
        MudletApp::getQSettings()->sync();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QVERIFY2(mudlet::self()->experiencedMudletPlayer(), "the first-run invitation would hide the games list these cases pick from");

        QVERIFY(QDir().mkpath(MudletApp::getMudletPath(enums::profileHomePath, mProfileName)));
        QVERIFY(MudletApp::writeProfileData(mProfileName, qsl("url"), mLocalhost).first);
        QVERIFY(MudletApp::writeProfileData(mProfileName, qsl("port"), QString::number(mpServer->serverPort())).first);

        mudlet::self()->startAutoLogin({});
        QVERIFY(QTest::qWaitFor(
                []() {
                    return mudlet::self()->mpConnectionDialog && mudlet::self()->mpConnectionDialog->isVisible();
                },
                5000));
    }

    void cleanupTestCase()
    {
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
        delete mpServer;
        mpServer = nullptr;
        delete mudlet::self();
    }

    // Loading a profile takes long enough for a second click to land on the
    // buttons of a dialog that is on its way out. PR #5313.
    void test_theDialogIsOffScreenBeforeTheProfileLoads()
    {
        QVERIFY2(dialog(), "No connection dialog to test against");
        QVERIFY(selectTestProfile());
        QVERIFY2(dialog()->isVisible(), "The dialog has to start out on screen for this case to mean anything");

        // stays true if the load never happens, which fails the case too
        mDialogOnScreenDuringTheLoad = true;
        const auto measurement = connect(mudlet::self(), &mudlet::signal_hostCreated, this, [this]() {
            mDialogAliveDuringTheLoad = (dialog() != nullptr);
            mDialogOnScreenDuringTheLoad = dialog() && dialog()->isVisible();
        });

        dialog()->slot_load(); // the Offline button
        disconnect(measurement);

        QVERIFY2(HostManager::self()->getHost(mProfileName), "The profile never loaded, so nothing was measured");
        QVERIFY2(mDialogAliveDuringTheLoad, "The dialog was already gone when the profile loaded, so its visibility was never measured");
        QVERIFY2(!mDialogOnScreenDuringTheLoad, "The dialog was still on screen while the profile loaded");
    }

    // With the profile open, the dialog has to leave it alone and just
    // reconnect it rather than run the load a second time. PR #7778.
    void test_askingForTheOpenProfileAgainReconnectsIt()
    {
        QVERIFY2(openTheTestProfile(), "The test profile could not be opened, so there is nothing for the dialog to be asked about twice");
        QVERIFY2(reopenDialog(), "No dialog to test against");
        QVERIFY(selectTestProfile());
        auto* pHost = mudlet::self()->getActiveHost();
        QVERIFY2(pHost && pHost->getName() == mProfileName, "The test profile is not the open one, so this case would take another path");

        QSignalSpy connections(mpServer, &QTcpServer::newConnection);
        QSignalSpy loads(dialog(), &dlgConnectionProfiles::signal_load_profile);
        // an unattached spy counts zero for ever, which is what this case reads as success
        QVERIFY(connections.isValid());
        QVERIFY(loads.isValid());

        dialog()->slot_load(); // the Offline button

        QVERIFY2(QTest::qWaitFor(
                         [&connections]() {
                             return !connections.isEmpty();
                         },
                         15000),
                 "Asking for the open profile again did not reconnect it");
        QCOMPARE(loads.count(), 0);
    }
};

#include "ConnectionDialogOpenProfileTest.moc"
MUDLET_GROUPED_TEST_MAIN(ConnectionDialogOpenProfileTest)
