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
 * The "Same settings, new look!" card at the top of whichever settings page is
 * showing. It stands on every opening of the settings until it is dismissed:
 * only the "Got it" button records that it has been seen.
 *
 * The flag behind it lives in the shared Mudlet.ini rather than in a profile,
 * which is why the config root has to be this process's own - see
 * test_theBannerFlagIsReadFromThisTestsOwnSettings.
 *
 * Run with: ctest -R SettingsBannerTest -V
 */

#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QBoxLayout>
#include <QFrame>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "SettingsTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"

#include "GroupedTest.h"

class SettingsBannerTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    const QString mProfileName = qsl("SettingsBanner-Test");
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = qsl("localhost");
    const QString mBannerSeenKey = qsl("settingsRedesignBannerSeen");

    static void deleteProfileDirectory(const QString& profileName) { TestSettings::deleteProfileDirectory(profileName); }

    QFrame* banner() const { return mpPreferences->findChild<QFrame*>(qsl("settingsMigrationBanner")); }

    // The widget every card of one category page is laid out in
    QWidget* columnOf(const QString& key) const
    {
        auto* pPage = TestSettings::pageOf(mpPreferences, key);
        return pPage ? pPage->widget() : nullptr;
    }

    void selectCategory(const QString& key)
    {
        auto* pList = TestSettings::sidebar(mpPreferences);
        QVERIFY2(pList, "the settings shell has no category sidebar");
        for (int row = 0, rows = pList->count(); row < rows; ++row) {
            if (pList->item(row)->data(Qt::UserRole).toString() == key) {
                pList->setCurrentRow(row);
                QCoreApplication::processEvents();
                return;
            }
        }
        QFAIL(qPrintable(qsl("no sidebar item for category '%1'").arg(key)));
    }

    void openPreferences()
    {
        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        mpPreferences->resize(1060, 760);
        mpPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpPreferences));
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
    }

    void init() { mudlet::getQSettings()->remove(mBannerSeenKey); }

    void cleanup()
    {
        delete mpPreferences;
        mpPreferences = nullptr;
        mudlet::getQSettings()->remove(mBannerSeenKey);
    }

    // The rest of this file writes and reads a key that decides whether a real
    // user ever sees the banner, so it has to be certain it is not writing the
    // developer's own settings file
    void test_theBannerFlagIsReadFromThisTestsOwnSettings()
    {
        const QString settingsFile = mudlet::getQSettings()->fileName();
        QVERIFY2(settingsFile.startsWith(mConfigDir.path()), qPrintable(qsl("the settings this test writes are in %1, not under its temporary config root %2").arg(settingsFile, mConfigDir.path())));
        QCOMPARE(settingsFile, qsl("%1/mudlet/Mudlet.ini").arg(mConfigDir.path()));
    }

    void test_aFreshInstallShowsTheBanner()
    {
        openPreferences();

        QFrame* pBanner = banner();
        QVERIFY2(pBanner, "the migration banner was not built on a fresh installation");
        QVERIFY2(pBanner->isVisible(), "the migration banner was built but not shown");
        QWidget* pGeneralColumn = columnOf(qsl("general"));
        QVERIFY2(pGeneralColumn, "the General page is not there under the object name this looks it up by");
        QVERIFY2(pGeneralColumn->isAncestorOf(pBanner), "the migration banner is not at the top of the page the dialog opens on");
        QCOMPARE(pGeneralColumn->layout()->indexOf(pBanner), 0);
    }

    // Whichever page is showing carries it, because a deep link or a
    // remembered category can open the dialog anywhere but General
    void test_theBannerRidesAlongToASecondCategory()
    {
        openPreferences();
        QFrame* pBanner = banner();
        QVERIFY(pBanner);

        selectCategory(qsl("mapper"));

        QWidget* pMapperColumn = columnOf(qsl("mapper"));
        QVERIFY(pMapperColumn);
        QVERIFY2(pMapperColumn->isAncestorOf(pBanner), "the migration banner stayed on the General page instead of following the category being shown");
        QCOMPARE(pMapperColumn->layout()->indexOf(pBanner), 0);
        QVERIFY2(pBanner->isVisible(), "the migration banner arrived on the second page hidden");
        QCOMPARE(columnOf(qsl("general"))->layout()->indexOf(pBanner), -1);
    }

    void test_dismissingTheBannerHidesItAndRemembersThat()
    {
        openPreferences();
        QFrame* pBanner = banner();
        QVERIFY(pBanner);

        auto* pDismiss = pBanner->findChild<QPushButton*>(qsl("settingsMigrationBannerDismiss"));
        QVERIFY2(pDismiss, "the migration banner has no button to dismiss it with");
        pDismiss->click();

        QVERIFY2(pBanner->isHidden(), "dismissing the banner left it on screen");
        QVERIFY2(mudlet::getQSettings()->value(mBannerSeenKey, false).toBool(), "dismissing the banner did not record that it had been seen");
    }

    // One "Got it" is meant to be the end of it everywhere, not just on the
    // page it was clicked from
    void test_dismissingTheBannerTakesItOffEveryPage()
    {
        openPreferences();
        QFrame* pBanner = banner();
        QVERIFY(pBanner);
        pBanner->findChild<QPushButton*>(qsl("settingsMigrationBannerDismiss"))->click();

        for (const QString& key : {qsl("appearance"), qsl("mapper"), qsl("general")}) {
            selectCategory(key);
            QWidget* pColumn = columnOf(key);
            QVERIFY(pColumn);
            QVERIFY2(pColumn->layout()->indexOf(pBanner) < 0, qPrintable(qsl("the dismissed banner came back on the '%1' page").arg(key)));
        }
        QVERIFY2(pBanner->isHidden(), "the dismissed banner is still showing somewhere");
    }

    void test_aLaterDialogDoesNotShowTheBanner()
    {
        mudlet::getQSettings()->setValue(mBannerSeenKey, true);
        openPreferences();

        QVERIFY2(!banner(), "the migration banner came back after it had been dismissed");
    }
};

#include "SettingsBannerTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsBannerTest)
