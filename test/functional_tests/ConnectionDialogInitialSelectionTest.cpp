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
 * Which profile the connection dialog picks when it opens, and whether the
 * connection details describe that same profile. Profiles that were made but
 * never connected carry no dated save to pick the most recent of, so with more
 * than one of them the dialog used to open with nothing picked at all (a single
 * one is picked by the lone-row fallback in fillout_form(), which is why this
 * test makes two) - and the games list then made its own first row current when
 * it took the keyboard focus, filling the connection details with that row's
 * game while leaving it unselected. See issue #10872.
 *
 * Run with: ctest -R ConnectionDialogInitialSelectionTest -V
 */

#include "PortableModeTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include <QtTest/QtTest>

#include <QLineEdit>
#include <QListWidget>
#include <chrono>
#include <filesystem> // NOLINT(build/c++17) - Qt cannot set the time of a directory, see dateSaveHoursAgo() below
#include <system_error>

#include "GroupedTest.h"

class ConnectionDialogInitialSelectionTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mXdgDir;
    QByteArray mSavedXdg;

    // the games tab bar's indices, which the dialog keeps private
    static constexpr int scmMyGamesTab = 0;
    static constexpr int scmAllGamesTab = 1;

    // The least a profile made in the dialog and never connected leaves on
    // disk: a folder, and the connection details if they were filled in. The
    // names are chosen so the one with a server address sorts first - the games
    // list appends profiles in the profiles/ directory's name order, so A- is
    // the row the dialog should pick, and B- proves it did not simply take the
    // only row there was
    const QString mFirstProfile = qsl("A-ConnDialogSelection");
    const QString mSecondProfile = qsl("B-ConnDialogSelection");
    const QString mFirstProfileUrl = qsl("127.0.0.1");
    // for the cases below, which rearrange the profiles directory
    const QString mOlderSavedProfile = qsl("C-ConnDialogSelectionOlder");
    const QString mNewerSavedProfile = qsl("D-ConnDialogSelectionNewer");

    dlgConnectionProfiles* dialog() const { return mudlet::self()->mpConnectionDialog.data(); }

    bool makeProfileFolder(const QString& name) const { return QDir().mkpath(MudletApp::getMudletPath(enums::profileHomePath, name)); }
    bool makeSavedGame(const QString& name) const { return QDir().mkpath(MudletApp::getMudletPath(enums::profileXmlFilesPath, name)); }
    bool removeProfile(const QString& name) const { return QDir(MudletApp::getMudletPath(enums::profileHomePath, name)).removeRecursively(); }

    // fillout_form() dates a save by the modification time of the profile's
    // current/ folder, and two folders made one after the other can carry the
    // same time - QFileInfo reads no finer than a millisecond, and a
    // filesystem's own clock can be coarser still - so a case that needs one
    // save to be older than another dates them itself rather than waiting and
    // hoping they land apart. Qt can only set the time of a file it has open
    // and a directory cannot be opened, hence the standard library here
    bool dateSaveHoursAgo(const QString& name, const int hours) const
    {
        // cleanPath() drops the trailing separator getMudletPath() leaves on
        // a directory, which not every platform takes when opening one
        const std::filesystem::path save(QDir::cleanPath(MudletApp::getMudletPath(enums::profileXmlFilesPath, name)).toStdU16String());
        std::error_code error;
        const auto written = std::filesystem::last_write_time(save, error);
        if (error) {
            return false;
        }
        std::filesystem::last_write_time(save, written - std::chrono::hours(hours), error);
        return !error;
    }

    void showGamesTab(const int tab) const { MudletApp::getQSettings()->setValue(qsl("connectionDialogActiveTab"), tab); }

    // the dialog makes its pick in fillout_form(), so a case that only asks
    // what was picked does not have to put a dialog on screen
    QString profilePickedByAFreshDialog() const
    {
        auto* pDialog = new dlgConnectionProfiles();
        pDialog->fillout_form();
        const auto* pCurrentItem = pDialog->listWidget_profiles->currentItem();
        const auto picked = pCurrentItem ? pCurrentItem->data(dlgConnectionProfiles::csmNameRole).toString() : QString();
        delete pDialog;
        return picked;
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

        mudlet::start();
        mudlet::self()->setupConfig();
        QVERIFY(MudletApp::getMudletPath(enums::profilesPath).startsWith(mXdgDir.path()));

        QVERIFY(MudletApp::writeProfileData(mFirstProfile, qsl("url"), mFirstProfileUrl).first);
        QVERIFY(MudletApp::writeProfileData(mFirstProfile, qsl("port"), qsl("4000")).first);
        QVERIFY(makeProfileFolder(mSecondProfile));
        // no save has ever been written, so there is no current/ folder for
        // fillout_form() to date
        QVERIFY(!QFileInfo::exists(MudletApp::getMudletPath(enums::profileXmlFilesPath, mFirstProfile)));
        QVERIFY(!QFileInfo::exists(MudletApp::getMudletPath(enums::profileXmlFilesPath, mSecondProfile)));

        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        showGamesTab(scmMyGamesTab);
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
        delete mudlet::self();
    }

    void test_theDetailsPaneDescribesThePickedProfile()
    {
        QVERIFY2(dialog(), "No connection dialog to test against");
        auto* pList = dialog()->listWidget_profiles;

        QVERIFY2(pList->count() > 1, "A lone row is picked by fillout_form()'s single-profile fallback whatever else happens, so one row would prove nothing");
        auto* pCurrentItem = pList->currentItem();
        QVERIFY2(pCurrentItem, "The games list has no current item at all - the dialog picked no profile and the list never took the keyboard focus");
        QVERIFY2(pList->selectedItems().contains(pCurrentItem), "The profile the connection details describe is not the one shown as picked");

        // nothing distinguishes profiles that were never connected, so the
        // first one listed is the pick
        const auto pickedProfile = pCurrentItem->data(dlgConnectionProfiles::csmNameRole).toString();
        QCOMPARE(pickedProfile, mFirstProfile);
        QCOMPARE(dialog()->profile_name_entry->text(), mFirstProfile);
        QCOMPARE(dialog()->host_name_entry->text(), mFirstProfileUrl);
    }

    // re-filling the form is what switching games tabs does, and it has to
    // leave the details describing the picked profile rather than blank
    void test_refillingTheFormKeepsTheDetailsOnThePickedProfile()
    {
        QVERIFY2(dialog(), "No connection dialog to test against");

        dialog()->fillout_form();
        dialog()->fillout_form();

        QCOMPARE(dialog()->profile_name_entry->text(), mFirstProfile);
        QCOMPARE(dialog()->host_name_entry->text(), mFirstProfileUrl);
    }

    // the pick that was there before still has to win: the dialog opens on the
    // game that was played last, not on the first one listed
    void test_theMostRecentlySavedProfileIsStillPicked()
    {
        QVERIFY(makeSavedGame(mOlderSavedProfile));
        QVERIFY(makeSavedGame(mNewerSavedProfile));
        QVERIFY(dateSaveHoursAgo(mOlderSavedProfile, 2));
        QVERIFY(dateSaveHoursAgo(mNewerSavedProfile, 1));
        const auto olderSave = QFileInfo(MudletApp::getMudletPath(enums::profileXmlFilesPath, mOlderSavedProfile)).lastModified();
        const auto newerSave = QFileInfo(MudletApp::getMudletPath(enums::profileXmlFilesPath, mNewerSavedProfile)).lastModified();
        QVERIFY2(newerSave > olderSave, "The two saved games cannot be told apart by date, so this case would prove nothing");

        showGamesTab(scmMyGamesTab);
        QCOMPARE(profilePickedByAFreshDialog(), mNewerSavedProfile);
    }

    // the self-test entry is a testing aid rather than a game, so it is not
    // what the dialog should offer to connect to
    void test_theSelfTestEntryIsNotPickedOverAGame()
    {
        QVERIFY(removeProfile(mOlderSavedProfile));
        QVERIFY(removeProfile(mNewerSavedProfile));
        // a self-test folder on disk lists the entry from the game catalog in
        // any build, and the catalog is listed ahead of the player's own
        // profiles
        QVERIFY(makeProfileFolder(dlgConnectionProfiles::scmSelfTestProfile));

        showGamesTab(scmMyGamesTab);
        QCOMPARE(profilePickedByAFreshDialog(), mFirstProfile);
    }

    // "All games" always lists the tutorial, which is picked before the
    // never-connected fallback is reached
    void test_allGamesStillOpensOnTheTutorial()
    {
        showGamesTab(scmAllGamesTab);
        QCOMPARE(profilePickedByAFreshDialog(), qsl("Mudlet Tutorial"));
    }
};

#include "ConnectionDialogInitialSelectionTest.moc"
MUDLET_GROUPED_TEST_MAIN(ConnectionDialogInitialSelectionTest)
