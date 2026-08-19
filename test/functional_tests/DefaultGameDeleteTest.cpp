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
 * Deleting a pre-installed game's profile from the "My games" tab of the
 * connection dialog must only remove that profile's local data - the game has
 * to remain available in the "All games" catalog afterwards. It used to also
 * vanish from "All games" because the deletion recorded the game in the
 * deletedDefaultMuds blocklist which fillout_form() applied to both tabs.
 *
 * Run with: ctest -R DefaultGameDeleteTest -V
 */

#include <QtTest/QtTest>

#include <QTabBar>
#include <QTabWidget>

#include "PortableModeTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "TGameDetails.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

class DefaultGameDeleteTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    const QString mGame = qsl("Mudlet Tutorial");
    // slot_itemClicked() ignores the same profile picked twice in quick
    // succession, so re-selecting one takes a detour through another game
    const QString mOtherGame = qsl("Achaea");
    const QString mSelfTest = dlgConnectionProfiles::scmSelfTestProfile;

    // the dialog holds two QTabBars: the games-list one, created directly on
    // the dialog, and QTabWidget's internal one - the parent check tells them
    // apart without matching translated tab text
    QTabBar* gamesTabBar(dlgConnectionProfiles* dlg) const
    {
        const auto tabBars = dlg->findChildren<QTabBar*>();
        for (auto* tabBar : tabBars) {
            if (!qobject_cast<QTabWidget*>(tabBar->parentWidget())) {
                return tabBar;
            }
        }
        return nullptr;
    }

    bool gameListed(dlgConnectionProfiles* dlg, const QString& game) const { return !dlg->findData(*dlg->listWidget_profiles, game, dlgConnectionProfiles::csmNameRole).isEmpty(); }

    // returns false rather than QVERIFYing: a failure here has to stop the
    // caller, which a QVERIFY in a helper would not do
    bool selectProfile(dlgConnectionProfiles* dlg, const QString& game) const
    {
        const auto items = dlg->findData(*dlg->listWidget_profiles, game, dlgConnectionProfiles::csmNameRole);
        if (items.isEmpty()) {
            return false;
        }
        dlg->listWidget_profiles->setCurrentItem(items.first());
        return dlg->listWidget_profiles->currentItem() == items.first();
    }

    // a second profile on disk keeps the dialog out of its first-launch state,
    // where fillout_form() hides the games list along with the Remove button
    // and the notification area
    bool makeProfileOnDisk(const QString& game) const { return QDir().mkpath(mudlet::getMudletPath(enums::profileHomePath, game)); }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        // pre-create $XDG_CONFIG_HOME/mudlet/profiles so setupConfig() adopts it
        // and the test never touches the real profiles or settings
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        QVERIFY2(TGameDetails::keys().contains(mGame), "expected pre-installed game missing from TGameDetails");
    }

    void cleanupTestCase()
    {
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_deletedDefaultGameStaysInAllGames()
    {
        // an on-disk profile dir makes the game appear under "My games"; an
        // empty one means slot_deleteProfile() deletes it without raising the
        // confirmation dialog
        QVERIFY(QDir().mkpath(mudlet::getMudletPath(enums::profileHomePath, mGame)));

        auto* dlg = new dlgConnectionProfiles();
        dlg->show();
        dlg->fillout_form();
        auto* tabBar = gamesTabBar(dlg);
        QVERIFY(tabBar);

        tabBar->setCurrentIndex(0); // "My games"
        dlg->fillout_form();
        QVERIFY2(gameListed(dlg, mGame), "game with an on-disk profile should show under 'My games'");

        // having no sub-directory of its own - nothing but the connection
        // details the dialog wrote there - is what makes slot_deleteProfile()
        // skip the confirmation dialog; assert it so a change there fails loudly
        QVERIFY(QDir(mudlet::getMudletPath(enums::profileHomePath, mGame)).entryList(QDir::Dirs | QDir::NoDotAndDotDot).isEmpty());
        const auto items = dlg->findData(*dlg->listWidget_profiles, mGame, dlgConnectionProfiles::csmNameRole);
        dlg->listWidget_profiles->setCurrentItem(items.first());
        dlg->slot_deleteProfile();

        QVERIFY2(!gameListed(dlg, mGame), "deleted game should no longer show under 'My games'");
        QVERIFY2(!QDir(mudlet::getMudletPath(enums::profileHomePath, mGame)).exists(), "profile data should be removed from disk");

        tabBar->setCurrentIndex(1); // "All games", refills the list
        QVERIFY2(gameListed(dlg, mGame), "a deleted pre-installed game must still be offered under 'All games'");

        dlg->deleteLater();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    // users who deleted a pre-installed game before this fix have it recorded
    // in the deletedDefaultMuds blocklist; it must not keep the game out of
    // the catalog
    void test_legacyBlocklistedGameStillShownInAllGames()
    {
        mudlet::self()->mpSettings->setValue(qsl("deletedDefaultMuds"), QStringList{mGame});

        auto* dlg = new dlgConnectionProfiles();
        dlg->show();
        auto* tabBar = gamesTabBar(dlg);
        QVERIFY(tabBar);
        // the dialog already opens on "All games" (tab choice persisted by the
        // previous test), so setCurrentIndex() fires no currentChanged and the
        // explicit refill is load-bearing
        tabBar->setCurrentIndex(1); // "All games"
        dlg->fillout_form();

        QVERIFY2(gameListed(dlg, mGame), "a game on the legacy deletedDefaultMuds blocklist must still be offered under 'All games'");

        dlg->deleteLater();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    // the self-test entry is not a game: debug builds offer it without any
    // profile data on disk, so unlike a real pre-installed game it has to
    // stay dismissed once deleted
    void test_deletedSelfTestStaysHidden()
    {
        mudlet::self()->mpSettings->setValue(qsl("deletedDefaultMuds"), QStringList{mSelfTest});

        auto* dlg = new dlgConnectionProfiles();
        dlg->show();
        auto* tabBar = gamesTabBar(dlg);
        QVERIFY(tabBar);
        tabBar->setCurrentIndex(1); // "All games"
        dlg->fillout_form();

        QVERIFY2(!gameListed(dlg, mSelfTest), "a deleted self-test entry must not come back under 'All games'");

        tabBar->setCurrentIndex(0); // "My games"
        dlg->fillout_form();
        QVERIFY2(!gameListed(dlg, mSelfTest), "a deleted self-test entry must not come back under 'My games'");

        dlg->deleteLater();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    // "Remove" deletes a profile's own data, and the catalog offers a
    // pre-installed game whether or not there is any - so on a game nobody has
    // played the button would have nothing at all to do
    void test_removeIsNotOfferedForAGameWithoutProfileData()
    {
        mudlet::self()->mpSettings->setValue(qsl("deletedDefaultMuds"), QStringList{});
        QVERIFY(!QDir(mudlet::getMudletPath(enums::profileHomePath, mGame)).exists());
        QVERIFY(makeProfileOnDisk(mOtherGame));

        auto* dlg = new dlgConnectionProfiles();
        dlg->show();
        auto* tabBar = gamesTabBar(dlg);
        QVERIFY(tabBar);
        tabBar->setCurrentIndex(1); // "All games"
        dlg->fillout_form();

        QVERIFY(selectProfile(dlg, mOtherGame));
        QVERIFY(selectProfile(dlg, mGame));
        QVERIFY2(dlg->remove_profile_button->isVisibleTo(dlg), "the button has to be on screen for its state to mean anything");
        QVERIFY2(!dlg->remove_profile_button->isEnabled(), "a game with no profile data of its own has nothing to remove");
        QVERIFY2(!dlg->remove_profile_button->toolTip().isEmpty(), "the disabled button has to say why it is disabled");

        QVERIFY(makeProfileOnDisk(mGame));
        QVERIFY(selectProfile(dlg, mOtherGame));
        QVERIFY(selectProfile(dlg, mGame));
        QVERIFY2(dlg->remove_profile_button->isEnabled(), "a game with profile data on disk must stay removable");

        QVERIFY(QDir(mudlet::getMudletPath(enums::profileHomePath, mGame)).removeRecursively());
        QVERIFY(QDir(mudlet::getMudletPath(enums::profileHomePath, mOtherGame)).removeRecursively());
        dlg->deleteLater();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    // a profile of one's own has no catalog entry to fall back on, so removing
    // it always takes it out of the list - even before it has been saved
    void test_removeStaysOfferedForAnUnsavedNewProfile()
    {
        auto* dlg = new dlgConnectionProfiles();
        dlg->show();
        dlg->fillout_form();

        dlg->slot_addProfile();

        QVERIFY(dlg->listWidget_profiles->currentItem());
        QVERIFY2(dlg->remove_profile_button->isEnabled(), "a new profile that has not been saved yet must still be removable from the list");

        dlg->deleteLater();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    // the blocklist only keeps the self-test entry away, so recording a game in
    // it does nothing but grow the settings file
    void test_removingAGameRecordsNothingAndSaysWhatItDid()
    {
        mudlet::self()->mpSettings->setValue(qsl("deletedDefaultMuds"), QStringList{});
        QVERIFY(makeProfileOnDisk(mGame));
        QVERIFY(makeProfileOnDisk(mOtherGame));

        auto* dlg = new dlgConnectionProfiles();
        dlg->show();
        auto* tabBar = gamesTabBar(dlg);
        QVERIFY(tabBar);
        tabBar->setCurrentIndex(1); // "All games"
        dlg->fillout_form();

        QVERIFY(selectProfile(dlg, mGame));
        dlg->slot_deleteProfile();

        QVERIFY2(!QDir(mudlet::getMudletPath(enums::profileHomePath, mGame)).exists(), "profile data should be removed from disk");
        QVERIFY2(gameListed(dlg, mGame), "the catalog still offers the game, which is what makes the removal invisible");
        QVERIFY2(!mudlet::self()->mpSettings->value(qsl("deletedDefaultMuds"), QStringList()).toStringList().contains(mGame), "a removed game must not be recorded in a list nothing reads for games");
        QVERIFY2(dlg->notificationArea->isVisibleTo(dlg), "the confirmation has to be on screen");
        QVERIFY2(dlg->notificationAreaIconLabelInformation->isVisibleTo(dlg), "a completed removal is news, not a warning");
        QVERIFY2(dlg->notificationAreaMessageBox->text().contains(mGame), "the removal has to be confirmed when the list cannot show it");
        QVERIFY2(!dlg->remove_profile_button->isEnabled(), "with the data gone there is nothing left for a second click to remove");
        const QString confirmation = dlg->notificationAreaMessageBox->text();

        // reaching a second removal takes two confirmations open at once, so
        // drive it directly: what matters is that it does not claim to have
        // removed anything a second time
        QVERIFY(selectProfile(dlg, mOtherGame));
        QVERIFY(selectProfile(dlg, mGame));
        dlg->slot_deleteProfile();
        QVERIFY(dlg->notificationAreaMessageBox->text().contains(mGame));
        QVERIFY2(dlg->notificationAreaMessageBox->text() != confirmation, "a removal that found nothing must not repeat the confirmation");

        QVERIFY(QDir(mudlet::getMudletPath(enums::profileHomePath, mOtherGame)).removeRecursively());
        dlg->deleteLater();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    // removing the self-test entry is how it gets dismissed, and that does need
    // recording: nothing else would keep it out of the list
    void test_removingTheSelfTestEntryIsStillRecorded()
    {
        mudlet::self()->mpSettings->setValue(qsl("deletedDefaultMuds"), QStringList{});

        auto* dlg = new dlgConnectionProfiles();
        dlg->show();
        auto* tabBar = gamesTabBar(dlg);
        QVERIFY(tabBar);
        tabBar->setCurrentIndex(1); // "All games"
        dlg->fillout_form();

        // the catalog offers the self-test entry whatever the build type; only
        // its extra "My games" entry is debug-only
        QVERIFY(gameListed(dlg, mSelfTest));
        QVERIFY(selectProfile(dlg, mSelfTest));
        QVERIFY2(dlg->remove_profile_button->isEnabled(), "the self-test entry has to stay dismissable");
        dlg->slot_deleteProfile();

        QVERIFY(mudlet::self()->mpSettings->value(qsl("deletedDefaultMuds"), QStringList()).toStringList().contains(mSelfTest));
        QVERIFY2(!gameListed(dlg, mSelfTest), "a dismissed self-test entry must not come back");

        dlg->deleteLater();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    // debug builds add the self-test entry to "My games" themselves; "All
    // games" already gets it from TGameDetails, so it must not be listed twice
    void test_selfTestListedOnce()
    {
        mudlet::self()->mpSettings->setValue(qsl("deletedDefaultMuds"), QStringList{});

        auto* dlg = new dlgConnectionProfiles();
        dlg->show();
        auto* tabBar = gamesTabBar(dlg);
        QVERIFY(tabBar);

        for (const int tab : {1, 0}) { // "All games", then "My games"
            tabBar->setCurrentIndex(tab);
            dlg->fillout_form();
            const auto items = dlg->findData(*dlg->listWidget_profiles, mSelfTest, dlgConnectionProfiles::csmNameRole);
#if defined(QT_DEBUG)
            QCOMPARE(items.size(), 1);
#else
            QVERIFY2(items.size() <= 1, "the self-test entry must never be listed twice");
#endif
        }

        dlg->deleteLater();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
};

#include "DefaultGameDeleteTest.moc"
MUDLET_GROUPED_TEST_MAIN(DefaultGameDeleteTest)
