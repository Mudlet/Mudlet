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
 * What the connection dialog does while a keychain read has not answered yet:
 * it stays on screen and says so, and the Connect toolbar button can always
 * bring it back. MUDLET_TEST_MODE keeps CredentialManager on file storage, so
 * the outstanding read is stood in for by the flag the real one sets.
 *
 * Run with: ctest -R ConnectionDialogKeychainWaitTest -V
 */

#include "PortableModeTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include <QtTest/QtTest>

#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <chrono>

#include "GroupedTest.h"

using namespace std::chrono_literals;

class ConnectionDialogKeychainWaitTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mXdgDir;
    QByteArray mSavedXdg;

    const QString mProfileName = qsl("ConnDialogKeychain-Test");

    dlgConnectionProfiles* dialog() const { return mudlet::self()->mpConnectionDialog.data(); }

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

        mudlet::start();
        mudlet::self()->setupConfig();
        QVERIFY(mudlet::getMudletPath(enums::profilesPath).startsWith(mXdgDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        mudlet::self()->startAutoLogin({});
        // the dialog is only shown from a queued lambda, so the pointer turning
        // up is not enough
        QVERIFY(QTest::qWaitFor(
                []() {
                    return mudlet::self()->mpConnectionDialog && mudlet::self()->mpConnectionDialog->isVisible();
                },
                5000));
    }

    void cleanupTestCase()
    {
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
        delete mudlet::self();
    }

    // Connect has to put the dialog back on screen, not just raise a hidden one
    void test_showConnectionDialogRevealsAHiddenDialog()
    {
        auto* dlg = dialog();
        QVERIFY2(dlg, "No connection dialog to test against");

        dlg->setVisible(false);
        QVERIFY2(!dlg->isVisible(), "The dialog would not hide, so this test cannot cover the reported behaviour");

        mudlet::self()->slot_showConnectionDialog();
        QTest::qWait(100ms);

        QVERIFY2(dialog() == dlg, "A second connection dialog was created instead of the existing one being used");
        QVERIFY2(dlg->isVisible(), "The existing dialog was left hidden, so the Connect button does nothing for the rest of the session");
    }

    // Must stay last: the queued load closes the dialog the other tests need.
    void test_pendingKeychainReadKeepsTheDialogUp()
    {
        auto* dlg = dialog();
        QVERIFY2(dlg, "No connection dialog to test against");

        if (!dlg->isVisible()) {
            dlg->show();
            QTest::qWait(100ms);
        }
        QVERIFY(dlg->isVisible());

        // set behind the signals: slot_updateName() would run the whole validation and profile
        // save, which is not what is under test here
        {
            const QSignalBlocker blocker(dlg->profile_name_entry);
            dlg->profile_name_entry->setText(mProfileName);
        }
        dlg->validName = true;
        dlg->validUrl = true;
        dlg->validPort = true;
        dlg->connect_button->setEnabled(true);
        dlg->offline_button->setEnabled(true);
        dlg->listWidget_profiles->setEnabled(true);
        dlg->clearNotificationArea();

        // stands in for the read that slot_loadPasswordAsync() would have started
        dlg->mKeychainOperationInProgress = true;

        dlg->accept();

        QVERIFY2(dlg->isVisible(), "The dialog hid itself while the keychain read was still outstanding");
        QVERIFY2(!dlg->connect_button->isEnabled(), "Connect stayed usable while the keychain read was outstanding");
        QVERIFY2(!dlg->offline_button->isEnabled(), "Offline stayed usable while the keychain read was outstanding");
        QVERIFY2(!dlg->listWidget_profiles->isEnabled(),
                 "The profile list stayed usable while the keychain read was outstanding, so another profile could be picked that would never load its password");
        QVERIFY2(dlg->mKeychainWaitShown, "The dialog is not in its waiting-for-the-keychain state");
        QVERIFY2(!dlg->notificationAreaMessageBox->text().isEmpty(), "The wait is not explained anywhere in the dialog");
        QCOMPARE(dlg->mPendingProfileLoad, mProfileName);
        QVERIFY2(dlg->mPendingConnect, "Connect queued a load that would not connect");

        // what the keychain callback does once the read answers
        dlg->mKeychainOperationInProgress = false;
        // the profile load itself is another test's business: loadProfile() returns at once on an
        // empty name, which keeps this from starting a real profile
        {
            const QSignalBlocker blocker(dlg->profile_name_entry);
            dlg->profile_name_entry->clear();
        }

        QVERIFY2(dlg->completePendingProfileLoad(mProfileName), "The queued load did not run when the keychain read completed");

        // the dialog is on its way out (WA_DeleteOnClose), but deleteLater() cannot have run yet
        QVERIFY2(!dlg->isVisible(), "The dialog stayed on screen after the queued load ran");
        QVERIFY2(dlg->connect_button->isEnabled(), "Connect was left disabled");
        QVERIFY2(dlg->offline_button->isEnabled(), "Offline was left disabled");
        QVERIFY2(dlg->listWidget_profiles->isEnabled(), "The profile list was left disabled");
        QVERIFY2(!dlg->mKeychainWaitShown, "The dialog is still in its waiting-for-the-keychain state");
        QVERIFY2(dlg->mPendingProfileLoad.isEmpty(), "The queued load was left queued");
        QVERIFY2(!dlg->completePendingProfileLoad(mProfileName), "A keychain answer arriving late ran the queued load a second time");
    }
};

#include "ConnectionDialogKeychainWaitTest.moc"
MUDLET_GROUPED_TEST_MAIN(ConnectionDialogKeychainWaitTest)
