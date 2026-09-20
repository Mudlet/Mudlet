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
 * The connection dialog draws each entry of its games list as an icon and
 * nothing else - the name is carried in the item's data, not its text. An entry
 * whose icon never gets set is therefore an empty row: still selectable, still
 * filling in the connection details beside the list, but with nothing at all to
 * see or click. The "Mudlet self-test" entry has no artwork of its own, so that
 * is exactly what it was, and so is any entry whose artwork cannot be read.
 *
 * A spec cannot reach this: the connection dialog is C++ only, with no Lua way
 * to open it or to read its list.
 *
 * Run with: ctest -R SelfTestProfileIconTest -V
 */

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QtTest/QtTest>

#include <QListWidget>
#include <QTabBar>

#include "MudletInstanceCoordinator.h"
#include "MudletPaths.h"
#include "PortableModeTestHelper.h"
#include "TGameDetails.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

class SelfTestProfileIconTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    const QString mSelfTest = dlgConnectionProfiles::scmSelfTestProfile;
    const QString mGameWithArtwork = qsl("Mudlet Tutorial");
    const QString mArtworkPath = qsl(":/icons/mudlet-tutorial.png");

    QListWidgetItem* listedItem(dlgConnectionProfiles* pDialog, const QString& game) const
    {
        const auto items = pDialog->findData(*pDialog->listWidget_profiles, game, dlgConnectionProfiles::csmNameRole);
        return items.isEmpty() ? nullptr : items.first();
    }

    void verifyEntryIsVisible(dlgConnectionProfiles* pDialog, const QString& game, const QString& where)
    {
        QListWidgetItem* pItem = listedItem(pDialog, game);
        QVERIFY2(pItem, qPrintable(qsl("'%1' is not listed under %2 at all").arg(game, where)));
        const QIcon icon = pItem->icon();
        QVERIFY2(!icon.isNull(), qPrintable(qsl("'%1' is listed under %2 without an icon, so its row is blank").arg(game, where)));

        // a plate the user can read the entry from, rather than a flat fill:
        // the list draws no text, so this is all there is to tell rows apart
        const QImage plate = icon.pixmap(pDialog->listWidget_profiles->iconSize()).toImage();
        QCOMPARE(plate.size(), QSize(120, 30));
        QSet<QRgb> colors;
        for (int y = 0; y < plate.height(); ++y) {
            for (int x = 0; x < plate.width(); ++x) {
                colors.insert(plate.pixel(x, y));
            }
        }
        QVERIFY2(colors.size() > 4, qPrintable(qsl("'%1' draws %2 color(s) under %3, so its row still identifies nothing").arg(game, QString::number(colors.size()), where)));
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletPaths::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        mudlet::self()->mpSettings->setValue(qsl("deletedDefaultMuds"), QStringList{});

        QVERIFY2(TGameDetails::keys().contains(mSelfTest), "the self-test entry is missing from the games catalog");
        if (!(*TGameDetails::findGame(mSelfTest)).icon.isEmpty()) {
            QSKIP("the self-test entry now has artwork of its own, so the fallback plate is no longer reachable from it");
        }
    }

    void cleanupTestCase()
    {
        if (mudlet::self()) {
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // with the profile on disk the entry comes from the games catalog, which is
    // where an entry without artwork was left iconless
    void test_theSelfTestEntryIsVisibleWithProfileDataOnDisk()
    {
        // an entry the catalog gives no artwork is not a failure, so it must
        // not be reported as one
        QTest::failOnWarning(QRegularExpression(qsl("doesn't have a valid icon")));
        QVERIFY(QDir().mkpath(MudletPaths::getMudletPath(enums::profileHomePath, mSelfTest)));

        auto* pDialog = new dlgConnectionProfiles();
        pDialog->show();
        auto* pTabBar = pDialog->findChild<QTabBar*>(qsl("gamesTabBar"));
        QVERIFY(pTabBar);

        pTabBar->setCurrentIndex(0); // "My games"
        pDialog->fillout_form();
        verifyEntryIsVisible(pDialog, mSelfTest, qsl("'My games'"));

        pTabBar->setCurrentIndex(1); // "All games"
        pDialog->fillout_form();
        verifyEntryIsVisible(pDialog, mSelfTest, qsl("'All games'"));

        pDialog->deleteLater();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    // and with nothing on disk it is added by the debug-build-only branch of
    // fillout_form(), which built its item by hand
    void test_theSelfTestEntryIsVisibleWithNoProfileDataOnDisk()
    {
        QDir(MudletPaths::getMudletPath(enums::profileHomePath, mSelfTest)).removeRecursively();
        QVERIFY(!QDir(MudletPaths::getMudletPath(enums::profileHomePath, mSelfTest)).exists());

        auto* pDialog = new dlgConnectionProfiles();
        pDialog->show();
        auto* pTabBar = pDialog->findChild<QTabBar*>(qsl("gamesTabBar"));
        QVERIFY(pTabBar);

        pTabBar->setCurrentIndex(0); // "My games"
        pDialog->fillout_form();
#if defined(QT_DEBUG)
        verifyEntryIsVisible(pDialog, mSelfTest, qsl("'My games'"));
#else
        QVERIFY2(!listedItem(pDialog, mSelfTest), "outside a debug build 'My games' lists the self-test entry only once it has profile data on disk - 'All games' lists it either way");
#endif

        // "All games" offers it from the catalog in every build
        pTabBar->setCurrentIndex(1); // "All games"
        pDialog->fillout_form();
        verifyEntryIsVisible(pDialog, mSelfTest, qsl("'All games'"));

        pDialog->deleteLater();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    // the plate stands in for artwork that is missing, and for nothing else
    void test_aGameWithArtworkStillDrawsIt()
    {
        const QPixmap artwork(mArtworkPath);
        QVERIFY2(!artwork.isNull(), "SETUP: the artwork resource did not load, so this case compares nothing");

        auto* pDialog = new dlgConnectionProfiles();
        pDialog->show();
        auto* pTabBar = pDialog->findChild<QTabBar*>(qsl("gamesTabBar"));
        QVERIFY(pTabBar);
        pTabBar->setCurrentIndex(1); // "All games"
        pDialog->fillout_form();

        QListWidgetItem* pItem = listedItem(pDialog, mGameWithArtwork);
        QVERIFY2(pItem, qPrintable(qsl("'%1' is not listed under 'All games' at all").arg(mGameWithArtwork)));
        const QImage drawn = pItem->icon().pixmap(QSize(120, 30)).toImage();
        const QImage expected = (artwork.width() == 120 ? artwork : artwork.scaled(QSize(120, 30), Qt::IgnoreAspectRatio, Qt::SmoothTransformation)).toImage();
        QVERIFY2(drawn == expected, qPrintable(qsl("'%1' is no longer drawn with its own artwork").arg(mGameWithArtwork)));

        pDialog->deleteLater();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    // artwork that fails to load is now drawn as a plate rather than as a
    // blank row, so nothing in the dialog would show that a game lost its own
    void test_everyGameInTheCatalogHasArtworkThatLoads()
    {
        for (const auto& game : TGameDetails::scmDefaultGames) {
            if (game.icon.isEmpty()) {
                QCOMPARE(game.name, mSelfTest); // the only entry the catalog gives no artwork
                continue;
            }
            QVERIFY2(!QPixmap(game.icon).isNull(), qPrintable(qsl("'%1' names artwork at '%2' that does not load").arg(game.name, game.icon)));
        }
    }

    // artwork that was named but could not be read is a fault, not an entry
    // without artwork: the row is still drawn, but the failure is reported
    void test_artworkThatCannotBeReadIsDrawnAsAPlateAndReported()
    {
        auto* pDialog = new dlgConnectionProfiles();
        pDialog->show();

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("doesn't have a valid icon")));
        auto* pItem = new QListWidgetItem();
        pDialog->setupMudProfile(pItem, qsl("Broken Artwork Game"), QString(), qsl(":/icons/there-is-no-such-icon.png"));

        QVERIFY2(!pItem->icon().isNull(), "a game whose artwork could not be read was left without an icon");
        QVERIFY2(pItem->toolTip().contains(qsl("artwork")), qPrintable(qsl("nothing in the entry says its artwork is broken, only the tooltip '%1'").arg(pItem->toolTip())));

        pDialog->deleteLater();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    // hasCustomIcon() can only tell that the file is there, so a profileicon
    // that is empty or is not an image reaches the same dead end
    void test_aProfileIconThatCannotBeReadIsDrawnAsAPlateAndReported()
    {
        const QString profileName = qsl("Test-BrokenProfileIcon");
        QVERIFY(QDir().mkpath(MudletPaths::getMudletPath(enums::profileHomePath, profileName)));
        QFile iconFile(MudletPaths::getMudletPath(enums::profileDataItemPath, profileName, qsl("profileicon")));
        QVERIFY(iconFile.open(QIODevice::WriteOnly));
        iconFile.close();
        QVERIFY2(iconFile.size() == 0, "SETUP: the stand-in for a corrupt icon file is not empty");

        auto* pDialog = new dlgConnectionProfiles();
        pDialog->show();

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("has an icon file that could not be read")));
        auto* pItem = new QListWidgetItem();
        pDialog->setupMudProfile(pItem, profileName, QString(), QString());

        QVERIFY2(!pItem->icon().isNull(), "a profile whose stored icon could not be read was left without one");
        QVERIFY2(pItem->toolTip().contains(qsl("artwork")), qPrintable(qsl("nothing in the entry says its icon is broken, only the tooltip '%1'").arg(pItem->toolTip())));

        QDir(MudletPaths::getMudletPath(enums::profileHomePath, profileName)).removeRecursively();
        pDialog->deleteLater();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    // the tooltip is rich text, and a description is the plain words of whoever
    // wrote it: markup left in one must not be able to eat the warning that
    // follows it, nor be acted on instead of shown
    void test_aDescriptionWithMarkupCannotSwallowTheArtworkWarning()
    {
        // an unclosed comment is the worst case - everything after it, the
        // warning included, becomes part of the comment
        const QString description = qsl("A friendly MUD <!-- blurb needs a rewrite");

        auto* pDialog = new dlgConnectionProfiles();
        pDialog->show();

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("doesn't have a valid icon")));
        auto* pItem = new QListWidgetItem();
        pDialog->setupMudProfile(pItem, qsl("Marked Up Game"), description, qsl(":/icons/there-is-no-such-icon.png"));

        const QString tooltip = pItem->toolTip();
        // a tooltip is shown through a label in Qt::AutoText mode, which asks
        // Qt::mightBeRichText(); the wrapper is what settles that either way
        QVERIFY2(Qt::mightBeRichText(tooltip), qPrintable(qsl("the tooltip is not rich text, so its own markup is shown raw: '%1'").arg(tooltip)));

        QTextDocument rendered;
        rendered.setHtml(tooltip);
        const QString shown = rendered.toPlainText();
        QVERIFY2(shown.contains(qsl("could not be read")), qPrintable(qsl("the description's markup swallowed the artwork warning - all the user sees is '%1'").arg(shown)));
        QVERIFY2(shown.contains(description), qPrintable(qsl("the description is not shown as the plain text it is - the user sees '%1'").arg(shown)));

        pDialog->deleteLater();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
};

#include "SelfTestProfileIconTest.moc"
MUDLET_GROUPED_TEST_MAIN(SelfTestProfileIconTest)
