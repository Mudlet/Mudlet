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
 * Removing a profile from the connection dialog must never reach outside that
 * one profile's folder. A name like "." addresses the profiles directory
 * itself and ".." the whole Mudlet configuration directory, so a name that is
 * not a folder of its own must never reach removeRecursively(). Also covers
 * the confirmation the user gets before any of their data goes, and the names
 * that must keep working.
 *
 * Run with: ctest -R ProfileDeletionSafetyTest -V
 */

#include <QtTest/QtTest>

#include "MudletInstanceCoordinator.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

class ProfileDeletionSafetyTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    const QString mKeeper = qsl("QA Keeper");

    QString profilePath(const QString& profile) const { return mudlet::getMudletPath(enums::profileHomePath, profile); }

    // setupConfig() consults portable.txt ahead of the XDG logic; skip rather
    // than run against an unexpected config dir (see ConfigDirOverrideTest)
    bool portableMarkerPresent() const
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    void makeProfileWithSavedGame(const QString& profile) const
    {
        QVERIFY(QDir().mkpath(mudlet::getMudletPath(enums::profileXmlFilesPath, profile)));
        QFile savedGame(qsl("%1/%2.xml").arg(mudlet::getMudletPath(enums::profileXmlFilesPath, profile), profile));
        QVERIFY(savedGame.open(QIODevice::WriteOnly));
        savedGame.write("<MudletPackage></MudletPackage>");
        savedGame.close();
    }

    // The list takes ownership, and rebuilding it drops the entry again
    void addListEntry(dlgConnectionProfiles* dlg, const QString& profile) const
    {
        auto* item = new QListWidgetItem();
        item->setData(dlgConnectionProfiles::csmNameRole, profile);
        dlg->listWidget_profiles->insertItem(0, item);
        dlg->listWidget_profiles->setCurrentItem(item);
    }

    // slot_itemClicked() ignores a repeat of the profile it was last given
    // within 100ms, and that guard is static, so it outlives the dialog
    void selectProfile(dlgConnectionProfiles* dlg, const QString& profile) const
    {
        const auto items = dlg->findData(*dlg->listWidget_profiles, profile, dlgConnectionProfiles::csmNameRole);
        QVERIFY2(!items.isEmpty(), qPrintable(qsl("profile '%1' was not listed").arg(profile)));
        dlg->listWidget_profiles->setCurrentItem(items.first());
        QTest::qWait(120);
        dlg->slot_itemClicked(items.first());
    }

    QDialog* confirmation(dlgConnectionProfiles* dlg) const { return dlg->findChild<QDialog*>(qsl("delete_profile_confirmation")); }

    // The .ui wires the delete button's clicked() to the dialog's accept()
    void confirmRemovalOf(dlgConnectionProfiles* dlg, const QString& profile) const
    {
        auto* confirmationDialog = confirmation(dlg);
        QVERIFY(confirmationDialog);
        auto* nameEntry = confirmationDialog->findChild<QLineEdit*>(qsl("delete_profile_lineedit"));
        auto* deleteButton = confirmationDialog->findChild<QPushButton*>(qsl("delete_button"));
        QVERIFY(nameEntry && deleteButton);

        nameEntry->setText(profile.left(profile.size() - 1));
        QVERIFY2(!deleteButton->isEnabled(), "a partial profile name enabled the delete button");

        nameEntry->setText(profile);
        QVERIFY2(deleteButton->isEnabled(), "typing the profile name did not enable this confirmation's delete button");
        deleteButton->click();
    }

    void removeProfileAndConfirm(dlgConnectionProfiles* dlg, const QString& profile) const
    {
        dlg->slot_deleteProfile();
        if (confirmation(dlg)) {
            confirmRemovalOf(dlg, profile);
        }
    }

    dlgConnectionProfiles* openDialog() const
    {
        auto* dlg = new dlgConnectionProfiles();
        dlg->show();
        dlg->fillout_form();
        return dlg;
    }

    void closeDialog(dlgConnectionProfiles* dlg) const
    {
        dlg->deleteLater();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    // The Connect and Offline buttons are the dialog's only AcceptRole buttons
    bool acceptButtonsEnabled(dlgConnectionProfiles* dlg) const
    {
        for (auto* button : dlg->dialog_buttonbox->buttons()) {
            if (dlg->dialog_buttonbox->buttonRole(button) == QDialogButtonBox::AcceptRole && !button->isEnabled()) {
                return false;
            }
        }
        return true;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - cannot redirect the config dir for this test");
        }

        QVERIFY(mConfigDir.isValid());
        // $XDG_CONFIG_HOME/mudlet/profiles is the opt-in that makes setupConfig()
        // adopt it, so the test never goes near the user's own profiles
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        makeProfileWithSavedGame(mKeeper);
        mudlet::self()->writeProfileData(mKeeper, qsl("url"), qsl("mudlet.org"));
        mudlet::self()->writeProfileData(mKeeper, qsl("port"), qsl("23"));
    }

    void cleanupTestCase()
    {
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_removingCurrentDirectoryProfileKeepsEveryProfile()
    {
        auto* dlg = openDialog();
        addListEntry(dlg, qsl("."));

        dlg->slot_deleteProfile();
        QVERIFY2(confirmation(dlg), "removal went ahead without asking");
        confirmRemovalOf(dlg, qsl("."));

        QVERIFY2(QDir(mudlet::getMudletPath(enums::profilesPath)).exists(), "the profiles directory was deleted");
        QVERIFY2(QDir(profilePath(mKeeper)).exists(), "an unrelated profile was deleted");
        QVERIFY2(QFile::exists(qsl("%1/%2.xml").arg(mudlet::getMudletPath(enums::profileXmlFilesPath, mKeeper), mKeeper)), "an unrelated profile's saved game was deleted");
        QVERIFY2(!dlg->notificationAreaMessageBox->text().isEmpty(), "the refusal was not reported to the user");

        closeDialog(dlg);
    }

    void test_removingParentDirectoryProfileKeepsTheConfigurationDirectory()
    {
        auto* dlg = openDialog();
        addListEntry(dlg, qsl(".."));

        dlg->slot_deleteProfile();
        QVERIFY2(confirmation(dlg), "removal went ahead without asking");
        confirmRemovalOf(dlg, qsl(".."));

        QVERIFY2(QDir(mudlet::getMudletPath(enums::mainPath)).exists(), "Mudlet's configuration directory was deleted");
        QVERIFY2(QDir(mudlet::getMudletPath(enums::profilesPath)).exists(), "the profiles directory was deleted");
        QVERIFY2(QDir(profilePath(mKeeper)).exists(), "an unrelated profile was deleted");
        QVERIFY2(!dlg->notificationAreaMessageBox->text().isEmpty(), "the refusal was not reported to the user");

        closeDialog(dlg);
    }

    void test_onlyDirectChildrenOfTheProfilesDirectoryAreProfiles()
    {
        const QString profilesPath = mudlet::getMudletPath(enums::profilesPath);

        QCOMPARE(dlgConnectionProfiles::profileFolderPath(profilesPath, mKeeper), qsl("%1/%2").arg(profilesPath, mKeeper));
        QVERIFY(dlgConnectionProfiles::profileFolderPath(profilesPath, qsl(".")).isEmpty());
        QVERIFY(dlgConnectionProfiles::profileFolderPath(profilesPath, qsl("..")).isEmpty());
        QVERIFY(dlgConnectionProfiles::profileFolderPath(profilesPath, qsl("%1/current").arg(mKeeper)).isEmpty());

        // a predefined game has no folder until it is saved
        const QString neverPlayed = qsl("QA Never Played");
        QVERIFY(!QDir(profilePath(neverPlayed)).exists());
        QCOMPARE(dlgConnectionProfiles::profileFolderPath(profilesPath, neverPlayed), qsl("%1/%2").arg(profilesPath, neverPlayed));
    }

    void test_profileWithOnlyAMapIsConfirmedBeforeRemoval()
    {
        const QString mapped = qsl("QA Mapped");
        QVERIFY(QDir().mkpath(mudlet::getMudletPath(enums::profileMapsPath, mapped)));
        QVERIFY(!QDir(mudlet::getMudletPath(enums::profileXmlFilesPath, mapped)).exists());

        auto* dlg = openDialog();
        selectProfile(dlg, mapped);

        dlg->slot_deleteProfile();
        QVERIFY2(confirmation(dlg), "removal went ahead without asking");
        QVERIFY2(QDir(profilePath(mapped)).exists(), "the profile was deleted before the user confirmed");

        confirmation(dlg)->reject();
        QVERIFY2(QDir(profilePath(mapped)).exists(), "the profile was deleted after the user cancelled");
        closeDialog(dlg);
    }

    void test_profileWithOnlyConnectionDetailsIsRemovedWithoutConfirmation()
    {
        const QString unplayed = qsl("QA Unplayed");
        QVERIFY(QDir().mkpath(profilePath(unplayed)));
        mudlet::self()->writeProfileData(unplayed, qsl("url"), qsl("mudlet.org"));
        mudlet::self()->writeProfileData(unplayed, qsl("port"), qsl("23"));

        auto* dlg = openDialog();
        selectProfile(dlg, unplayed);

        dlg->slot_deleteProfile();
        QVERIFY2(!confirmation(dlg), "a profile with nothing but connection details should not need confirming");
        QVERIFY2(!QDir(profilePath(unplayed)).exists(), "the profile was not removed");
        closeDialog(dlg);
    }

    // test_profileWithOnlyConnectionDetailsIsRemovedWithoutConfirmation only
    // holds while dlgConnectionProfiles::scmConnectionDetailFiles still covers
    // what the connection form writes, so fill one in the way a user does and
    // hold what lands on disk against that list
    void test_newProfileOnlyWritesListedConnectionDetails()
    {
        const QString unplayed = qsl("QA Just Set Up");
        QVERIFY(!QDir(profilePath(unplayed)).exists());

        auto* dlg = openDialog();
        dlg->slot_addProfile();
        dlg->profile_name_entry->setText(unplayed);
        // what leaving the name field does, and what creates the profile's
        // folder - it only gets that far synchronously because initTestCase()
        // turned secure password storage off, else it waits on the keychain
        dlg->slot_saveName();
        QVERIFY2(QDir(profilePath(unplayed)).exists(), "naming a new profile did not create its folder");

        // the rest of the connection form. The character name and the password
        // are left out on purpose: they are the user's own, so a profile
        // holding either is still confirmed - see the two cases below
        dlg->host_name_entry->setText(qsl("mudlet.org"));
        dlg->port_entry->setText(qsl("23"));
        dlg->port_ssl_tsl->setChecked(true);
        dlg->autologin_checkBox->setChecked(true);
        dlg->auto_reconnect->setChecked(true);
        dlg->mud_description_textedit->setPlainText(qsl("a game to try later"));

        // refilling the form by selecting the profile writes through the same
        // field signals, so the selection path is covered as well
        selectProfile(dlg, unplayed);

        const QStringList written = QDir(profilePath(unplayed)).entryList(QDir::Files | QDir::Hidden);
        // a field that stops saving would otherwise quietly shrink what the
        // check below covers, while still passing it
        for (const QString& expected : {qsl("url"), qsl("port"), qsl("ssl_tsl"), qsl("autologin"), qsl("autoreconnect"), qsl("description")}) {
            QVERIFY2(written.contains(expected), qPrintable(qsl("filling the form in no longer writes '%1', so this case has stopped covering it").arg(expected)));
        }
        for (const QString& fileName : written) {
            QVERIFY2(dlgConnectionProfiles::scmConnectionDetailFiles.contains(fileName),
                     qPrintable(qsl("setting a profile up wrote '%1', which dlgConnectionProfiles::scmConnectionDetailFiles does not list - add it there, or "
                                    "reconsider removing such a profile without confirmation")
                                        .arg(fileName)));
        }

        dlg->slot_deleteProfile();
        QVERIFY2(!confirmation(dlg), "a profile that was only ever set up should not need confirming");
        QVERIFY2(!QDir(profilePath(unplayed)).exists(), "the profile was not removed");
        closeDialog(dlg);
    }

    // A stored password sits loose in the folder rather than in a sub-directory
    void test_profileWithOnlyAStoredPasswordIsConfirmedBeforeRemoval()
    {
        const QString secretive = qsl("QA Secretive");
        QVERIFY(QDir().mkpath(profilePath(secretive)));
        mudlet::self()->writeProfileData(secretive, qsl("password"), qsl("hunter2"));
        QVERIFY(QDir(profilePath(secretive)).entryList(QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot).isEmpty());

        auto* dlg = openDialog();
        selectProfile(dlg, secretive);

        dlg->slot_deleteProfile();
        QVERIFY2(confirmation(dlg), "a profile holding a stored password was removed without asking");
        QVERIFY2(QDir(profilePath(secretive)).exists(), "the profile was deleted before the user confirmed");

        confirmation(dlg)->reject();
        closeDialog(dlg);
    }

    // The character name typed into the login field is the user's own text
    // rather than one of the connection details the shortcut waves through
    void test_profileWithACharacterNameIsConfirmedBeforeRemoval()
    {
        const QString named = qsl("QA Named");
        QVERIFY(QDir().mkpath(profilePath(named)));
        mudlet::self()->writeProfileData(named, qsl("url"), qsl("mudlet.org"));
        mudlet::self()->writeProfileData(named, qsl("login"), qsl("Aurelius"));

        auto* dlg = openDialog();
        selectProfile(dlg, named);

        dlg->slot_deleteProfile();
        QVERIFY2(confirmation(dlg), "a profile holding a character name was removed without asking");
        QVERIFY2(QDir(profilePath(named)).exists(), "the profile was deleted before the user confirmed");

        confirmation(dlg)->reject();
        closeDialog(dlg);
    }

    // Nothing stops a second confirmation being raised over the first
    void test_eachConfirmationRemovesItsOwnProfile()
    {
        const QString first = qsl("QA First");
        const QString second = qsl("QA Second");
        makeProfileWithSavedGame(first);
        makeProfileWithSavedGame(second);

        auto* dlg = openDialog();
        selectProfile(dlg, first);
        dlg->slot_deleteProfile();
        auto* firstConfirmation = confirmation(dlg);
        QVERIFY(firstConfirmation);

        selectProfile(dlg, second);
        dlg->slot_deleteProfile();
        const auto confirmations = dlg->findChildren<QDialog*>(qsl("delete_profile_confirmation"));
        QCOMPARE(confirmations.size(), 2);
        auto* secondConfirmation = confirmations.first() == firstConfirmation ? confirmations.last() : confirmations.first();

        auto* nameEntry = firstConfirmation->findChild<QLineEdit*>(qsl("delete_profile_lineedit"));
        auto* deleteButton = firstConfirmation->findChild<QPushButton*>(qsl("delete_button"));
        QVERIFY(nameEntry && deleteButton);
        nameEntry->setText(first);
        QVERIFY2(deleteButton->isEnabled(), "the first confirmation did not accept its own profile name");
        deleteButton->click();

        QVERIFY2(!QDir(profilePath(first)).exists(), "the first confirmation did not remove its own profile");
        QVERIFY2(QDir(profilePath(second)).exists(), "the first confirmation removed the second profile instead");

        secondConfirmation->reject();
        closeDialog(dlg);
    }

    void test_profileWithDataIsStillRemovable()
    {
        const QString doomed = qsl("QA Doomed");
        makeProfileWithSavedGame(doomed);

        auto* dlg = openDialog();
        selectProfile(dlg, doomed);

        dlg->slot_deleteProfile();
        QVERIFY2(confirmation(dlg), "removal of a profile with data went ahead without asking");
        QVERIFY2(QDir(profilePath(doomed)).exists(), "the profile was deleted before the user confirmed");

        confirmRemovalOf(dlg, doomed);

        QVERIFY2(!QDir(profilePath(doomed)).exists(), "the confirmed profile was not removed");
        QVERIFY2(QDir(profilePath(mKeeper)).exists(), "an unrelated profile was removed too");
        closeDialog(dlg);
    }

    // A name Mudlet would turn down as a new profile is still a profile on disk
    void test_nonAsciiProfileIsStillRemovable()
    {
        const QString doomed = qsl("Мудлет café");
        makeProfileWithSavedGame(doomed);

        auto* dlg = openDialog();
        selectProfile(dlg, doomed);

        removeProfileAndConfirm(dlg, doomed);

        QVERIFY2(!QDir(profilePath(doomed)).exists(), "the confirmed profile was not removed");
        QVERIFY2(QDir(profilePath(mKeeper)).exists(), "an unrelated profile was removed too");
        closeDialog(dlg);
    }

    // The confirmation is not modal, so the selection can move on behind it
    void test_confirmationRemovesTheProfileItNamed()
    {
        const QString doomed = qsl("QA Doomed Too");
        makeProfileWithSavedGame(doomed);

        auto* dlg = openDialog();
        selectProfile(dlg, doomed);
        dlg->slot_deleteProfile();
        QVERIFY(confirmation(dlg));

        selectProfile(dlg, mKeeper);

        confirmRemovalOf(dlg, doomed);

        QVERIFY2(QDir(profilePath(mKeeper)).exists(), "the newly selected profile was removed instead");
        QVERIFY2(!QDir(profilePath(doomed)).exists(), "the confirmed profile was not removed");
        closeDialog(dlg);
    }

    void test_typedNamesThatAreNotProfilesAreRejected_data()
    {
        QTest::addColumn<QString>("name");

        QTest::newRow("current directory") << qsl(".");
        QTest::newRow("parent directory") << qsl("..");
        QTest::newRow("embedded parent directory") << qsl("Achaea..Beta");
    }

    void test_typedNamesThatAreNotProfilesAreRejected()
    {
        QFETCH(QString, name);

        auto* dlg = openDialog();
        selectProfile(dlg, mKeeper);
        // else the assertion below also holds for an unrelated reason
        QVERIFY2(acceptButtonsEnabled(dlg), "the profile was already unusable before the name was edited");

        // setText() drives the same textChanged path as typing does
        dlg->profile_name_entry->setText(name);
        QVERIFY2(!acceptButtonsEnabled(dlg), qPrintable(qsl("'%1' was accepted as a profile name").arg(name)));
        QVERIFY(QDir(profilePath(mKeeper)).exists());

        dlg->profile_name_entry->setText(mKeeper);
        closeDialog(dlg);
    }

    // A folder on disk is the user's data whatever it is called
    void test_folderOnDiskWithATurnedDownNameIsStillUsable()
    {
        const QString awkward = qsl("QA..Dots");
        QVERIFY(!dlgConnectionProfiles::profileNameUsableAsIs(awkward));
        makeProfileWithSavedGame(awkward);
        mudlet::self()->writeProfileData(awkward, qsl("url"), qsl("mudlet.org"));
        mudlet::self()->writeProfileData(awkward, qsl("port"), qsl("23"));

        auto* dlg = openDialog();
        selectProfile(dlg, awkward);

        QCOMPARE(dlg->profile_name_entry->text(), awkward);
        QVERIFY2(acceptButtonsEnabled(dlg), "a profile folder already on disk was refused");
        QVERIFY2(QDir(profilePath(awkward)).exists(), "the folder was renamed behind the user's back");
        closeDialog(dlg);
    }

    void test_typedNamesThatAreProfilesAreAccepted_data()
    {
        QTest::addColumn<QString>("name");

        QTest::newRow("version number") << qsl("QA Game 2.0");
        QTest::newRow("parentheses") << qsl("QA Keeper (2)");
    }

    void test_typedNamesThatAreProfilesAreAccepted()
    {
        QFETCH(QString, name);

        auto* dlg = openDialog();
        selectProfile(dlg, mKeeper);

        dlg->profile_name_entry->setText(name);
        QCOMPARE(dlg->profile_name_entry->text(), name);
        QVERIFY2(acceptButtonsEnabled(dlg), qPrintable(qsl("'%1' was refused as a profile name").arg(name)));

        // ~QDialog fires editingFinished() into slot_saveName(), which renames
        dlg->profile_name_entry->setText(mKeeper);
        closeDialog(dlg);
    }
};

#include "ProfileDeletionSafetyTest.moc"
MUDLET_GROUPED_TEST_MAIN(ProfileDeletionSafetyTest)
