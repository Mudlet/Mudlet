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

#include "MudletInstanceCoordinator.h"
#include "TGameDetails.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForDefaultGameDeleteTest();

class DefaultGameDeleteTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    const QString mGame = qsl("Mudlet Tutorial");
    const QString mSelfTest = qsl("Mudlet self-test");

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

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForDefaultGameDeleteTest();

        QVERIFY(mConfigDir.isValid());
        // pre-create $XDG_CONFIG_HOME/mudlet so setupConfig() adopts it and the
        // test never touches the real profiles or settings
        QVERIFY(QDir().mkpath(qsl("%1/mudlet").arg(mConfigDir.path())));
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
        // an on-disk profile dir makes the game appear under "My games"; no
        // saved XMLs inside means slot_deleteProfile() deletes without raising
        // the confirmation dialog
        QVERIFY(QDir().mkpath(mudlet::getMudletPath(enums::profileHomePath, mGame)));

        auto* dlg = new dlgConnectionProfiles();
        dlg->show();
        dlg->fillout_form();
        auto* tabBar = gamesTabBar(dlg);
        QVERIFY(tabBar);

        tabBar->setCurrentIndex(0); // "My games"
        dlg->fillout_form();
        QVERIFY2(gameListed(dlg, mGame), "game with an on-disk profile should show under 'My games'");

        // no saved XMLs is what makes slot_deleteProfile() skip the
        // confirmation dialog; assert it so a change there fails loudly
        QVERIFY(!QDir(mudlet::getMudletPath(enums::profileXmlFilesPath, mGame)).exists());
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

void initializeQRCResourcesForDefaultGameDeleteTest()
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

#include "DefaultGameDeleteTest.moc"
QTEST_MAIN(DefaultGameDeleteTest)
