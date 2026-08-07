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
 * one profile's folder. A profile called "." named the profiles directory
 * itself and ".." named the whole Mudlet configuration directory, so removing
 * one wiped every profile the user had. Also covers the confirmation the user
 * gets before any of their data goes, and the names that must keep working.
 *
 * Run with: ctest -R ProfileDeletionSafetyTest -V
 */

#include <QtTest/QtTest>

#include "MudletInstanceCoordinator.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForProfileDeletionSafetyTest();

class ProfileDeletionSafetyTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    // the profile that must survive every attempt below
    const QString mKeeper = qsl("QA Keeper");

    QString profilePath(const QString& profile) const { return mudlet::getMudletPath(enums::profileHomePath, profile); }

    void makeProfileWithSavedGame(const QString& profile) const
    {
        QVERIFY(QDir().mkpath(mudlet::getMudletPath(enums::profileXmlFilesPath, profile)));
        QFile savedGame(qsl("%1/%2.xml").arg(mudlet::getMudletPath(enums::profileXmlFilesPath, profile), profile));
        QVERIFY(savedGame.open(QIODevice::WriteOnly));
        savedGame.write("<MudletPackage></MudletPackage>");
        savedGame.close();
    }

    // Adds a list entry for a profile that has no folder of its own, the way
    // the dialog does for a name being typed into it. This is the state the
    // user was left in after naming a new profile "." or "..".
    QListWidgetItem* addListEntry(dlgConnectionProfiles* dlg, const QString& profile) const
    {
        auto* item = new QListWidgetItem();
        item->setData(dlgConnectionProfiles::csmNameRole, profile);
        dlg->listWidget_profiles->insertItem(0, item);
        dlg->listWidget_profiles->setCurrentItem(item);
        return item;
    }

    // slot_itemClicked() ignores a repeat of the profile it was last given
    // within 100ms, and that guard is static - it outlives the dialog, so
    // consecutive tests picking the same profile have to wait it out
    void selectProfile(dlgConnectionProfiles* dlg, const QString& profile) const
    {
        const auto items = dlg->findData(*dlg->listWidget_profiles, profile, dlgConnectionProfiles::csmNameRole);
        QVERIFY2(!items.isEmpty(), qPrintable(qsl("profile '%1' was not listed").arg(profile)));
        dlg->listWidget_profiles->setCurrentItem(items.first());
        QTest::qWait(120);
        dlg->slot_itemClicked(items.first());
    }

    QDialog* confirmation(dlgConnectionProfiles* dlg) const { return dlg->findChild<QDialog*>(qsl("delete_profile_confirmation")); }

    // Answers a raised confirmation the way an intent-on-deleting user would:
    // by typing the profile name in, then pressing the delete button.
    void confirmRemovalOf(dlgConnectionProfiles* dlg, const QString& profile) const
    {
        auto* confirmationDialog = confirmation(dlg);
        QVERIFY(confirmationDialog);
        auto* nameEntry = confirmationDialog->findChild<QLineEdit*>(qsl("delete_profile_lineedit"));
        auto* deleteButton = confirmationDialog->findChild<QPushButton*>(qsl("delete_button"));
        QVERIFY(nameEntry && deleteButton);

        nameEntry->setText(profile);
        QVERIFY(deleteButton->isEnabled());
        confirmationDialog->accept();
    }

    // Presses Remove and answers the confirmation, if one is raised at all
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
        initializeQRCResourcesForProfileDeletionSafetyTest();

        QVERIFY(mConfigDir.isValid());
        // an existing $XDG_CONFIG_HOME/mudlet makes setupConfig() adopt it, so
        // the test never goes near the user's own profiles
        QVERIFY(QDir().mkpath(qsl("%1/mudlet").arg(mConfigDir.path())));
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
        auto* item = addListEntry(dlg, qsl("."));

        removeProfileAndConfirm(dlg, qsl("."));

        QVERIFY2(QDir(mudlet::getMudletPath(enums::profilesPath)).exists(), "the profiles directory was deleted");
        QVERIFY2(QDir(profilePath(mKeeper)).exists(), "an unrelated profile was deleted");
        QVERIFY2(QFile::exists(qsl("%1/%2.xml").arg(mudlet::getMudletPath(enums::profileXmlFilesPath, mKeeper), mKeeper)), "an unrelated profile's saved game was deleted");

        delete item;
        closeDialog(dlg);
    }

    void test_removingParentDirectoryProfileKeepsTheConfigurationDirectory()
    {
        auto* dlg = openDialog();
        auto* item = addListEntry(dlg, qsl(".."));

        removeProfileAndConfirm(dlg, qsl(".."));

        QVERIFY2(QDir(mudlet::getMudletPath(enums::mainPath)).exists(), "Mudlet's configuration directory was deleted");
        QVERIFY2(QDir(mudlet::getMudletPath(enums::profilesPath)).exists(), "the profiles directory was deleted");
        QVERIFY2(QDir(profilePath(mKeeper)).exists(), "an unrelated profile was deleted");

        delete item;
        closeDialog(dlg);
    }

    // Whatever a name resolves to, only a folder directly inside the profiles
    // directory is ever a profile
    void test_onlyDirectChildrenOfTheProfilesDirectoryAreProfiles()
    {
        const QString profilesPath = mudlet::getMudletPath(enums::profilesPath);

        QCOMPARE(dlgConnectionProfiles::profileFolderPath(profilesPath, mKeeper), qsl("%1/%2").arg(profilesPath, mKeeper));
        QVERIFY(dlgConnectionProfiles::profileFolderPath(profilesPath, qsl(".")).isEmpty());
        QVERIFY(dlgConnectionProfiles::profileFolderPath(profilesPath, qsl("..")).isEmpty());
        QVERIFY(dlgConnectionProfiles::profileFolderPath(profilesPath, qsl("%1/current").arg(mKeeper)).isEmpty());
    }

    // A profile whose folder holds no saved games but does hold something else
    // still has data to lose, so it has to be confirmed
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

    // A pre-installed game that was never played has nothing to lose - only
    // the connection details this dialog wrote into its folder - so removing
    // it goes ahead without a confirmation
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

    // A folder name Mudlet would not accept for a new profile is still a
    // profile once it is on disk, and removing it must work
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

    // The confirmation dialog is not modal, so the profile it names is the one
    // that gets removed even if the list selection moves on behind it
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

        // setText() drives the same textChanged path as typing does
        dlg->profile_name_entry->setText(name);
        QVERIFY2(!acceptButtonsEnabled(dlg), qPrintable(qsl("'%1' was accepted as a profile name").arg(name)));
        // the folder the profile came from must not have been renamed to it
        QVERIFY(QDir(profilePath(mKeeper)).exists());

        dlg->profile_name_entry->setText(mKeeper);
        closeDialog(dlg);
    }

    void test_typedNamesThatAreProfilesAreAccepted_data()
    {
        QTest::addColumn<QString>("name");

        QTest::newRow("version number") << qsl("Achaea 2.0");
        QTest::newRow("parentheses") << qsl("QA Keeper (2)");
        QTest::newRow("leading dot") << qsl(".hidden");
    }

    void test_typedNamesThatAreProfilesAreAccepted()
    {
        QFETCH(QString, name);

        auto* dlg = openDialog();
        selectProfile(dlg, mKeeper);

        dlg->profile_name_entry->setText(name);
        QCOMPARE(dlg->profile_name_entry->text(), name);
        QVERIFY2(acceptButtonsEnabled(dlg), qPrintable(qsl("'%1' was refused as a profile name").arg(name)));

        // put the on-disk name back so no rename can be left pending
        dlg->profile_name_entry->setText(mKeeper);
        closeDialog(dlg);
    }
};

void initializeQRCResourcesForProfileDeletionSafetyTest()
{
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
    qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
    qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    qInitResources_mudlet_fonts_posix();
#endif
#endif
    qInitResources_mudlet();
    qInitResources_qm();
}

#include "ProfileDeletionSafetyTest.moc"
QTEST_MAIN(ProfileDeletionSafetyTest)
