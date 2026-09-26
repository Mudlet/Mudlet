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
 * the outstanding read is stood in for by the state the real one sets, and its
 * answers by calls to the handlers the real one calls.
 *
 * Run with: ctest -R ConnectionDialogKeychainWaitTest -V
 */

#include "PortableModeTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
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
        QVERIFY(MudletApp::getMudletPath(enums::profilesPath).startsWith(mXdgDir.path()));
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

    // A read that timed out can still be answered afterwards, and by then the load it was holding
    // up has run without it: that second answer may fill an empty password field and nothing else.
    void test_aLateKeychainAnswerTouchesNothingButAnEmptyPasswordField()
    {
        // A dialog of its own: completing the queued load at the end closes whichever dialog it
        // runs on, and the shared one is what the case below needs. Never shown, so no keychain
        // read of its own is ever started.
        auto* dlg = new dlgConnectionProfiles(mudlet::self());

        // the late answer only considers a field whose profile is still the selected one, so the
        // list needs the entry that state is about - set behind the signals, which would otherwise
        // run the whole profile-selected handler
        {
            const QSignalBlocker blocker(dlg->listWidget_profiles);
            auto* profileItem = new QListWidgetItem(mProfileName, dlg->listWidget_profiles);
            profileItem->setData(dlgConnectionProfiles::csmNameRole, mProfileName);
            dlg->listWidget_profiles->setCurrentItem(profileItem);
        }
        {
            const QSignalBlocker blocker(dlg->character_password_entry);
            dlg->character_password_entry->setText(qsl("typed-while-waiting"));
        }

        dlg->mKeychainOperationProfile = mProfileName;
        dlg->mPendingProfileLoad = mProfileName;
        dlg->mPendingConnect = true;
        dlg->showKeychainWait();

        dlg->passwordArrivedLate(mProfileName, true, qsl("from-the-keychain"), QString());

        QCOMPARE(dlg->character_password_entry->text(), qsl("typed-while-waiting"));
        QCOMPARE(dlg->mKeychainOperationProfile, mProfileName);
        QCOMPARE(dlg->mPendingProfileLoad, mProfileName);
        QVERIFY2(dlg->mPendingConnect, "the late answer dropped the queued connection");
        QVERIFY2(dlg->mKeychainWaitShown, "the late answer ended a wait that is still going on");

        // and the read in flight, answering properly, still completes the queued load. loadProfile()
        // returns at once on an empty profile name, which keeps this from starting a real profile
        QVERIFY(dlg->profile_name_entry->text().isEmpty());
        dlg->passwordRetrieved(mProfileName, true, qsl("from-the-keychain"), QString());

        QVERIFY2(dlg->mKeychainOperationProfile.isEmpty(), "the answered read is still recorded as in flight");
        QVERIFY2(dlg->mPendingProfileLoad.isEmpty(), "the queued load was left queued");
        QVERIFY2(!dlg->mKeychainWaitShown, "the dialog is still in its waiting-for-the-keychain state");

        dlg->deleteLater();
    }

    // The other half of the late answer: a field still empty on a dialog nobody has pressed Connect
    // on yet gets the password, which a Connect then loads the profile with.
    void test_aLateKeychainAnswerFillsAnEmptyPasswordField()
    {
        auto* dlg = new dlgConnectionProfiles(mudlet::self());
        {
            const QSignalBlocker blocker(dlg->listWidget_profiles);
            auto* profileItem = new QListWidgetItem(mProfileName, dlg->listWidget_profiles);
            profileItem->setData(dlgConnectionProfiles::csmNameRole, mProfileName);
            dlg->listWidget_profiles->setCurrentItem(profileItem);
        }
        QVERIFY2(dlg->character_password_entry->text().isEmpty(), "a fresh dialog's password field is not empty, so this test cannot cover the refill");

        dlg->passwordArrivedLate(mProfileName, true, qsl("from-the-keychain"), QString());

        QCOMPARE(dlg->character_password_entry->text(), qsl("from-the-keychain"));
        dlg->deleteLater();
    }

    // A later read of the same profile that fails falls back on the password kept in the settings,
    // which is empty for a profile whose password lives in the keychain: it must not wipe the one an
    // earlier read handed over late.
    void test_aFailedReadKeepsAPasswordAlreadyInTheField()
    {
        auto* dlg = new dlgConnectionProfiles(mudlet::self());
        {
            const QSignalBlocker blocker(dlg->listWidget_profiles);
            auto* profileItem = new QListWidgetItem(mProfileName, dlg->listWidget_profiles);
            profileItem->setData(dlgConnectionProfiles::csmNameRole, mProfileName);
            dlg->listWidget_profiles->setCurrentItem(profileItem);
        }
        dlg->passwordArrivedLate(mProfileName, true, qsl("from-the-keychain"), QString());
        QCOMPARE(dlg->character_password_entry->text(), qsl("from-the-keychain"));

        dlg->mKeychainOperationProfile = mProfileName;
        dlg->passwordRetrieved(mProfileName, false, QString(), qsl("Operation timed out"));

        QCOMPARE(dlg->character_password_entry->text(), qsl("from-the-keychain"));
        dlg->deleteLater();
    }

    // An answer for a profile other than the queued one has nothing to run, but the wait it ends
    // took Connect, Offline, the profile list and the form away: they have to come back, or the
    // dialog is stuck the way an unanswered read used to leave it.
    void test_anAnswerForAnotherProfileHandsTheDialogBack()
    {
        auto* dlg = new dlgConnectionProfiles(mudlet::self());
        dlg->validName = true;
        dlg->validUrl = true;
        dlg->validPort = true;
        dlg->connect_button->setEnabled(true);
        dlg->offline_button->setEnabled(true);
        dlg->listWidget_profiles->setEnabled(true);

        dlg->mKeychainOperationProfile = qsl("some-other-profile");
        dlg->mPendingProfileLoad = mProfileName;
        dlg->mPendingConnect = true;
        dlg->showKeychainWait();
        QVERIFY2(!dlg->connect_button->isEnabled(), "the wait did not take Connect away, so this test cannot cover handing it back");

        dlg->passwordRetrieved(qsl("some-other-profile"), true, qsl("its-password"), QString());

        QVERIFY2(dlg->connect_button->isEnabled(), "Connect was left disabled");
        QVERIFY2(dlg->offline_button->isEnabled(), "Offline was left disabled");
        QVERIFY2(dlg->listWidget_profiles->isEnabled(), "The profile list was left disabled");
        QVERIFY2(dlg->tabWidget_connectionInfo->isEnabled(), "The connection details were left locked");
        QVERIFY2(dlg->profileAdminArea->isEnabled(), "The profile buttons were left locked");
        QVERIFY2(!dlg->mKeychainWaitShown, "the dialog is still in its waiting-for-the-keychain state");
        QVERIFY2(dlg->mPendingProfileLoad.isEmpty(), "a load queued for another profile was left queued");
        QVERIFY2(!dlg->mPendingConnect, "a connection queued for another profile was left queued");
        QVERIFY2(dlg->mKeychainOperationProfile.isEmpty(), "the answered read is still recorded as in flight");
        dlg->deleteLater();
    }

    // A read in flight holds up the load of its own profile only. Connect on another profile has
    // nothing to wait for, and queued behind that read it would be dropped when the read answers.
    void test_aReadForAnotherProfileDoesNotHoldUpALoad()
    {
        auto* dlg = new dlgConnectionProfiles(mudlet::self());
        dlg->mKeychainOperationProfile = qsl("profile-being-read");

        QVERIFY2(dlg->hasPendingKeychainOperation(qsl("profile-being-read")), "a load did not wait for the read of its own profile's password");
        QVERIFY2(!dlg->hasPendingKeychainOperation(mProfileName), "a load was held up behind the read of another profile's password");
        dlg->deleteLater();
    }

    // The wait locks the form: an edit would run validateProfile(), which hands Connect and Offline
    // back and clears the notice, and the queued load reads the fields as they are when it runs. A
    // field that loses the focus as it is locked still runs that validation, which must not end the
    // wait either.
    void test_theWaitLocksTheFormAndValidationCannotEndIt()
    {
        auto* dlg = new dlgConnectionProfiles(mudlet::self());
        {
            const QSignalBlocker listBlocker(dlg->listWidget_profiles);
            auto* profileItem = new QListWidgetItem(mProfileName, dlg->listWidget_profiles);
            profileItem->setData(dlgConnectionProfiles::csmNameRole, mProfileName);
            dlg->listWidget_profiles->setCurrentItem(profileItem);
            const QSignalBlocker nameBlocker(dlg->profile_name_entry);
            dlg->profile_name_entry->setText(mProfileName);
            const QSignalBlocker hostBlocker(dlg->host_name_entry);
            dlg->host_name_entry->setText(qsl("localhost"));
            const QSignalBlocker portBlocker(dlg->port_entry);
            dlg->port_entry->setText(qsl("4000"));
        }
        QVERIFY2(dlg->validateProfile(), "the form does not validate, so this test cannot show validation handing Connect back");

        dlg->mKeychainOperationProfile = mProfileName;
        dlg->mPendingProfileLoad = mProfileName;
        dlg->mPendingConnect = true;
        dlg->showKeychainWait();

        QVERIFY2(!dlg->tabWidget_connectionInfo->isEnabled(), "the connection details could still be edited during the wait");
        QVERIFY2(!dlg->profileAdminArea->isEnabled(), "profiles could still be added, copied or removed during the wait");

        dlg->validateProfile();
        QVERIFY2(!dlg->connect_button->isEnabled(), "validating the form during the wait handed Connect back");
        QVERIFY2(!dlg->offline_button->isEnabled(), "validating the form during the wait handed Offline back");
        QVERIFY2(!dlg->notificationAreaMessageBox->text().isEmpty(), "validating the form during the wait cleared the notice that explains it");

        dlg->abandonPendingProfileLoad();
        QVERIFY2(dlg->tabWidget_connectionInfo->isEnabled(), "the connection details were left locked after the wait");
        QVERIFY2(dlg->profileAdminArea->isEnabled(), "the profile buttons were left locked after the wait");
        dlg->deleteLater();
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
        dlg->mKeychainOperationProfile = mProfileName;

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
        dlg->mKeychainOperationProfile.clear();
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
