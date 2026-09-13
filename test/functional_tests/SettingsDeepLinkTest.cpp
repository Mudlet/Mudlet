/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers - mudlet@mudlet.org           *
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
 * mudlet::showOptionsDialog(tab, host) still takes a tab objectName, so every
 * caller names a tab of a QTabWidget that no longer exists.
 * dlgProfilePreferences::setTab() is the remap table that keeps those names
 * working, and the whole table is covered here.
 *
 * Run with: ctest -R SettingsDeepLinkTest -V
 */

#include <QDir>
#include <QFileInfo>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QScrollArea>
#include <QStackedWidget>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "SettingsTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"

#include "GroupedTest.h"

class SettingsDeepLinkTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    // The Editor page's theme refresh creates the cache directory before it
    // checks whether it has anything to fetch, so it runs on the cached branch
    // too - and without this it would be the developer's own ~/.cache
    QTemporaryDir mCacheDir;
    QByteArray mSavedXdgCache;
    QByteArray mSavedNoThemeDownload;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    const QString mProfileName = qsl("SettingsDeepLink-Test");
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = qsl("localhost");

    static void deleteProfileDirectory(const QString& profileName) { TestSettings::deleteProfileDirectory(profileName); }

    // tab_codeEditor lands on the Editor category, which reads the themes it
    // offers from this file. What keeps that visit off the network is
    // MUDLET_TEST_NO_THEME_DOWNLOAD rather than anything about this file.
    static void writeEditorThemesFile()
    {
        const QString file = mudlet::getMudletPath(enums::editorWidgetThemeJsonFile);
        QVERIFY(QDir().mkpath(QFileInfo(file).absolutePath()));
        QFile themes(file);
        QVERIFY(themes.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QVERIFY(themes.write("[]") == 2);
    }

    QListWidget* sidebar() const { return TestSettings::sidebar(mpPreferences); }

    QStackedWidget* stack() const { return TestSettings::stack(mpPreferences); }
    QScrollArea* pageOf(const QString& key) const { return TestSettings::pageOf(mpPreferences, key); }

    QString currentCategory() const
    {
        const QListWidgetItem* pItem = sidebar()->currentItem();
        return pItem ? pItem->data(Qt::UserRole).toString() : QString();
    }

    void openPreferences()
    {
        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        mpPreferences->resize(1060, 760);
        mpPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpPreferences));
        QVERIFY2(sidebar(), "the settings shell has no category sidebar");
    }

    // The pulse is deferred to the next event loop turn, because setTab() runs
    // before the dialog is shown
    QWidget* waitForSpotlight()
    {
        if (!QTest::qWaitFor(
                    [this]() {
                        return mpPreferences->findChild<QWidget*>(qsl("settingsSpotlight")) != nullptr;
                    },
                    5000)) {
            return nullptr;
        }
        return mpPreferences->findChild<QWidget*>(qsl("settingsSpotlight"));
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own - see the same block in
        // DialogTeardownTest for why sharing the developer's one does not work
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
        QVERIFY(mCacheDir.isValid());
        mSavedXdgCache = qgetenv("XDG_CACHE_HOME");
        qputenv("XDG_CACHE_HOME", mCacheDir.path().toUtf8());
        // Nothing here may reach github.com for the edbee themes
        mSavedNoThemeDownload = qgetenv("MUDLET_TEST_NO_THEME_DOWNLOAD");
        qputenv("MUDLET_TEST_NO_THEME_DOWNLOAD", "1");

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);
        writeEditorThemesFile();

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY2(mpHost, "No active host after profile creation");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
        mSavedXdgCache.isNull() ? qunsetenv("XDG_CACHE_HOME") : qputenv("XDG_CACHE_HOME", mSavedXdgCache);
        mSavedNoThemeDownload.isNull() ? qunsetenv("MUDLET_TEST_NO_THEME_DOWNLOAD") : qputenv("MUDLET_TEST_NO_THEME_DOWNLOAD", mSavedNoThemeDownload);
    }

    void init() { openPreferences(); }

    void cleanup()
    {
        delete mpPreferences;
        mpPreferences = nullptr;
    }

    // The whole remap table, not only the five names src/mudlet.cpp,
    // src/ctelnet.cpp, MapSymbolFontTest.cpp and DialogTeardownTest.cpp pass
    // today: the other seven are the ones nothing else would notice breaking.
    void test_everyLegacyTabNameLandsOnItsCategory()
    {
        const QList<std::pair<QString, QString>> remaps{{qsl("tab_general"), qsl("general")},
                                                        {qsl("tab_inputLine"), qsl("inputLine")},
                                                        {qsl("tab_display"), qsl("mainDisplay")},
                                                        {qsl("tab_displayColors"), qsl("mainDisplay")},
                                                        {qsl("tab_codeEditor"), qsl("editor")},
                                                        {qsl("tab_mapper"), qsl("mapper")},
                                                        {qsl("tab_mapperColors"), qsl("mapper")},
                                                        {qsl("tab_chat"), qsl("chat")},
                                                        {qsl("tab_connection"), qsl("privacy")},
                                                        {qsl("tab_shortcuts"), qsl("shortcuts")},
                                                        {qsl("tab_accessibility"), qsl("accessibility")},
                                                        {qsl("tab_specialOptions"), qsl("connection")}};

        for (const auto& [legacyTab, category] : remaps) {
            // ...from somewhere else each time, so that a remap doing nothing
            // at all cannot pass by leaving the dialog where it already was
            mpPreferences->setTab(category == qsl("advanced") ? qsl("general") : qsl("advanced"));
            mpPreferences->setTab(legacyTab);
            QCOMPARE(currentCategory(), category);
            QCOMPARE(stack()->currentWidget(), pageOf(category));
        }
    }

    // A name the remap table does not know falls back to General, rather than
    // selecting nothing at all
    void test_anUnrecognisedTargetLandsOnGeneral()
    {
        for (const QString& target : {qsl("tab_thereIsNoSuchTab"), QString(), qsl("tab_generalise"), qsl("nosuchcategory/nosuchcard")}) {
            mpPreferences->setTab(qsl("advanced"));
            QCOMPARE(currentCategory(), qsl("advanced"));
            mpPreferences->setTab(target);
            QCOMPARE(currentCategory(), qsl("general"));
        }
    }

    // The one live deep link: ctelnet.cpp opens the settings on tab_connection
    // when a certificate is refused, and what it means is the Secure connection
    // card rather than the top of the Privacy page
    void test_theCertificateDeepLinkSpotlightsTheSecureConnectionCard()
    {
        mpPreferences->setTab(qsl("tab_connection"));
        QCOMPARE(currentCategory(), qsl("privacy"));

        QWidget* pPulse = waitForSpotlight();
        QVERIFY2(pPulse, "the tab_connection deep link drew no spotlight");
        auto* pPage = pageOf(qsl("privacy"));
        QVERIFY2(pPage, "the Privacy page is not there under the object name this looks it up by");
        QCOMPARE(pPulse->parentWidget(), pPage->widget());

        QGroupBox* pCard = mpPreferences->groupBox_ssl;
        const QRect cardRect(pCard->mapTo(pPage->widget(), QPoint(0, 0)), pCard->size());
        QVERIFY2(pPulse->geometry().intersects(cardRect), "the spotlight was drawn somewhere other than over the Secure connection card");
    }

    // New-style targets are 'category' or 'category/cardObjectName', and the
    // card named has to be brought into the viewport rather than merely being
    // on the page that is shown
    void test_aNewStyleTargetScrollsItsCardIntoView()
    {
        mpPreferences->setTab(qsl("mapper"));
        auto* pPage = pageOf(qsl("mapper"));
        QVERIFY2(pPage, "the Mapper page is not there under the object name this looks it up by");
        QVERIFY2(pPage->verticalScrollBar()->maximum() > 0, "the Mapper page fits its viewport, so scrolling to a card could not be detected");
        pPage->verticalScrollBar()->setValue(0);

        mpPreferences->setTab(qsl("mapper/groupBox_playerRoomStyle"));
        QCOMPARE(currentCategory(), qsl("mapper"));
        // the scroll itself happens in the same deferred turn as the pulse
        QVERIFY2(waitForSpotlight(), "the new-style deep link drew no spotlight");

        QVERIFY2(pPage->verticalScrollBar()->value() > 0, "the page never scrolled, so the card was only reachable by hand");
        QGroupBox* pCard = mpPreferences->groupBox_playerRoomStyle;
        const QRect cardRect(pCard->mapTo(pPage->viewport(), QPoint(0, 0)), pCard->size());
        QVERIFY2(pPage->viewport()->rect().intersects(cardRect), "the Player room marker card is still outside the viewport");
    }

    // A deep link can arrive while a search is showing - a certificate is
    // refused, and what the dialog is showing is the results of whatever the
    // user was looking for. Leaving the results is what selecting the target
    // category does, so a link naming the category the sidebar is *already* on
    // has nothing to select and would otherwise leave the search up, with the
    // card the link meant on a page nobody can see.
    void test_aDeepLinkDuringASearchLeavesTheResults()
    {
        mpPreferences->setTab(qsl("privacy"));
        QCOMPARE(currentCategory(), qsl("privacy"));

        QLineEdit* pSearch = mpPreferences->findChild<QLineEdit*>(qsl("settingsSearchField"));
        QVERIFY2(pSearch, "the settings shell has no search field");
        QVERIFY2(TestSettings::search(mpPreferences, qsl("color")), "the search never ran");
        QCOMPARE(stack()->currentWidget(), pageOf(qsl("searchResults")));

        mpPreferences->setTab(qsl("tab_connection"));

        QVERIFY2(pSearch->text().isEmpty(), "the deep link left the query standing in the search field");
        QCOMPARE(currentCategory(), qsl("privacy"));
        QCOMPARE(stack()->currentWidget(), pageOf(qsl("privacy")));
        QVERIFY2(waitForSpotlight(), "the deep link that arrived during a search drew no spotlight");
    }

    // A subpage is deep-linkable in its own right: "category/subpageKey" goes
    // into it rather than landing on the category page with the row that opens
    // it, and the sidebar still shows which category that is.
    void test_aSubpageTargetGoesIntoTheSubpage()
    {
        mpPreferences->setTab(qsl("connection/protocols"));
        QCOMPARE(currentCategory(), qsl("connection"));
        QCOMPARE(stack()->currentWidget(), pageOf(qsl("connection_protocols")));
    }

    // ...and so is a card that lives on one: naming it takes the way in rather
    // than leaving the spotlight on a page nobody is looking at.
    void test_aCardOnASubpageIsReachedThroughItsSubpage()
    {
        mpPreferences->setTab(qsl("connection/card_protocolList"));
        QCOMPARE(currentCategory(), qsl("connection"));
        QCOMPARE(stack()->currentWidget(), pageOf(qsl("connection_protocols")));
        QVERIFY2(waitForSpotlight(), "the deep link to a card on a subpage drew no spotlight");
    }

    // The pulse is a widget laid over a card, transparent to the mouse but
    // still a child of the page. One left behind after its animation would
    // stack up, one per deep link, over pages the player goes on using.
    void test_theSpotlightTakesItselfAwayWhenItHasFinished()
    {
        mpPreferences->setTab(qsl("tab_connection"));
        QVERIFY2(waitForSpotlight(), "the deep link drew no spotlight, so there is nothing to see cleaned up");

        // The fade runs for 2.5s and the widget is deleted on the next turn
        // of the event loop after it
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return mpPreferences->findChild<QWidget*>(qsl("settingsSpotlight")) == nullptr;
                         },
                         8000),
                 "the spotlight was still a child of the dialog long after its animation had finished");
    }
};

#include "SettingsDeepLinkTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsDeepLinkTest)
