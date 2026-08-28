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
 * The settings dialog's shell: the .ui file's twelve-page QTabWidget is taken
 * apart at construction and rebuilt as a grouped sidebar over a stack of
 * scrolling card pages. None of that is reachable from Lua - it is private C++
 * inside a dialog - so it is covered here rather than by a spec.
 *
 * What this file pins down: every category exists, in the order the sidebar
 * groups are specified in, and carries the cards the mapping table gives it;
 * the tab widget and the old Save row are really gone from the layout; each
 * page keeps its own scroll position, whether or not one of its own controls
 * holds the keyboard focus as it is left; a wheel that passes over a spin box
 * scrolls the page rather than editing the setting; and the Editor page's
 * one-off theme refresh happens on the first visit only.
 *
 * Run with: ctest -R SettingsShellNavigationTest -V
 */

#include <QDir>
#include <QFileInfo>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QApplication>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QScrollArea>
#include <QSettings>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTranslator>
#include <QWheelEvent>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"

#include "GroupedTest.h"

// Brackets every string it is asked for, so that a case can tell what the
// dialog re-read on a language change from what it is still showing from
// before it - without depending on which translations this build shipped.
class BracketingTranslator : public QTranslator
{
public:
    QString translate(const char* context, const char* sourceText, const char* disambiguation, int n) const override
    {
        Q_UNUSED(context)
        Q_UNUSED(disambiguation)
        Q_UNUSED(n)
        return qsl("[%1]").arg(QString::fromUtf8(sourceText));
    }

    bool isEmpty() const override { return false; }
};

class SettingsShellNavigationTest : public QObject
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
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    const QString mProfileName = qsl("SettingsShellNavigation-Test");
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = qsl("localhost");

    // The sidebar groups: seven everyday categories, then five system ones
    static QStringList specOrderedCategories()
    {
        return {qsl("general"),
                qsl("appearance"),
                qsl("mainDisplay"),
                qsl("inputLine"),
                qsl("editor"),
                qsl("mapper"),
                qsl("chat"),
                qsl("connection"),
                qsl("privacy"),
                qsl("accessibility"),
                qsl("shortcuts"),
                qsl("advanced")};
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    // The first visit to the Editor category refreshes the edbee themes from
    // colorsublime, which is a network round trip. A themes file younger than
    // the update period sends maybeDownloadEditorThemes() down its cached
    // branch instead, so walking every category here reaches no network.
    void writeFreshEditorThemesFile()
    {
        const QString file = mudlet::getMudletPath(enums::editorWidgetThemeJsonFile);
        QVERIFY(QDir().mkpath(QFileInfo(file).absolutePath()));
        QFile themes(file);
        QVERIFY(themes.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QVERIFY(themes.write("[]") == 2);
    }

    QListWidget* sidebar() const { return mpPreferences->findChild<QListWidget*>(qsl("settingsCategoryList")); }

    QStackedWidget* stack() const { return mpPreferences->findChild<QStackedWidget*>(qsl("settingsStack")); }

    QScrollArea* pageOf(const QString& key) const { return mpPreferences->findChild<QScrollArea*>(qsl("settingsPage_%1").arg(key)); }

    int rowOf(const QString& key) const
    {
        QListWidget* pList = sidebar();
        for (int row = 0, rows = pList->count(); row < rows; ++row) {
            if (pList->item(row)->data(Qt::UserRole).toString() == key) {
                return row;
            }
        }
        return -1;
    }

    // Clicking rather than setting the current row: a click gives the sidebar
    // the keyboard focus first, which is what stops QStackedLayout handing the
    // focus from the outgoing page to the incoming one - and that hand-off
    // scrolls the page being left back to its top.
    void selectCategory(const QString& key)
    {
        QListWidget* pList = sidebar();
        const int row = rowOf(key);
        QVERIFY2(row >= 0, qPrintable(qsl("no sidebar item for category '%1'").arg(key)));
        QListWidgetItem* pItem = pList->item(row);
        pList->scrollToItem(pItem);
        QTest::mouseClick(pList->viewport(), Qt::LeftButton, Qt::NoModifier, pList->visualItemRect(pItem).center());
        QCOMPARE(pList->currentRow(), row);
    }

    void openPreferences()
    {
        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        mpPreferences->resize(1060, 760);
        mpPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpPreferences));
        QVERIFY2(sidebar(), "the settings shell has no category sidebar");
        QVERIFY2(stack(), "the settings shell has no category stack");
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
        writeFreshEditorThemesFile();

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
    }

    void init() { openPreferences(); }

    void cleanup()
    {
        delete mpPreferences;
        mpPreferences = nullptr;
    }

    // Every other case here rests on the tab strip being gone from the shell.
    // The tab widget itself survives, holding no pages, so what has to be
    // checked is that it is out of the layout rather than deleted.
    void test_theTabWidgetAndTheSaveRowAreOutOfTheLayout()
    {
        QCOMPARE(mpPreferences->vBoxLayout_main->indexOf(mpPreferences->tabWidget), -1);
        QCOMPARE(mpPreferences->vBoxLayout_main->indexOf(mpPreferences->widget_bottom), -1);
        QCOMPARE(mpPreferences->tabWidget->count(), 0);
        QVERIFY2(mpPreferences->tabWidget->isHidden(), "the emptied tab widget is still on screen");
        QVERIFY2(mpPreferences->widget_bottom->isHidden(), "the Save button row is still on screen");

        auto* pShell = mpPreferences->findChild<QWidget*>(qsl("settingsShell"));
        QVERIFY2(pShell, "the sidebar-and-cards shell was never built");
        QVERIFY2(mpPreferences->vBoxLayout_main->indexOf(pShell) >= 0, "the shell is not what the dialog's main layout holds");
    }

    // Twelve categories in three groups, the separators between them
    // unselectable, and the support link at the bottom which opens a browser
    // rather than a page of its own
    void test_theSidebarListsEveryCategoryInSpecOrder()
    {
        QListWidget* pList = sidebar();
        QStringList found;
        int separators = 0;
        for (int row = 0, rows = pList->count(); row < rows; ++row) {
            const QListWidgetItem* pItem = pList->item(row);
            const QString key = pItem->data(Qt::UserRole).toString();
            if (!key.isEmpty()) {
                found << key;
                continue;
            }
            if (pItem->flags() == Qt::NoItemFlags) {
                ++separators;
            }
        }
        QCOMPARE(found, specOrderedCategories());
        QCOMPARE(separators, 2);

        const QListWidgetItem* pSupport = pList->item(pList->count() - 1);
        QCOMPARE(pSupport->data(Qt::UserRole + 1).toString(), qsl("https://wiki.mudlet.org"));
        QVERIFY2(!(pSupport->flags() & Qt::ItemIsSelectable), "the support link is selectable, so it would look like a category with no page");
        QVERIFY2(pSupport->flags() & Qt::ItemIsEnabled, "the support link cannot be clicked");
    }

    // One card per category, so that a control landing on the wrong page - or a
    // page that was never built - is caught rather than only showing up in a
    // screenshot
    void test_eachCategoryShowsItsMarqueeCard()
    {
        const QList<std::pair<QString, QGroupBox*>> marqueeCards{{qsl("general"), mpPreferences->groupBox_logOptions},
                                                                 {qsl("appearance"), mpPreferences->groupBox_iconsAndToolbars},
                                                                 {qsl("mainDisplay"), mpPreferences->groupBox_displayColors},
                                                                 {qsl("inputLine"), mpPreferences->groupBox_input},
                                                                 {qsl("editor"), mpPreferences->groupBox_editorDisplayOptions},
                                                                 {qsl("mapper"), mpPreferences->groupBox_mapFiles},
                                                                 {qsl("chat"), mpPreferences->groupBox_MMCPOptions},
                                                                 {qsl("connection"), mpPreferences->groupBox_protocols},
                                                                 {qsl("privacy"), mpPreferences->groupBox_ssl},
                                                                 {qsl("accessibility"), mpPreferences->groupBox_accessibility},
                                                                 {qsl("shortcuts"), mpPreferences->groupBox_main_window_shortcuts},
                                                                 {qsl("advanced"), mpPreferences->groupBox_debug}};

        for (const auto& [key, pCard] : marqueeCards) {
            selectCategory(key);
            QScrollArea* pPage = pageOf(key);
            QVERIFY2(pPage, qPrintable(qsl("category '%1' has no page").arg(key)));
            QCOMPARE(stack()->currentWidget(), pPage);
            QCOMPARE(mpPreferences->findChild<QLabel*>(qsl("settingsPageTitle"))->text(), sidebar()->item(rowOf(key))->text());
            QVERIFY2(pPage->widget()->isAncestorOf(pCard), qPrintable(qsl("%1 is not on the '%2' page").arg(pCard->objectName(), key)));
            QVERIFY2(pCard->isVisible(), qPrintable(qsl("%1 is not showing on the '%2' page").arg(pCard->objectName(), key)));
        }
    }

    // Each page is its own scroll area, so walking away from one and coming
    // back has to land where it was left rather than at the top
    void test_eachCategoryKeepsItsOwnScrollPosition()
    {
        selectCategory(qsl("mapper"));
        QScrollBar* pMapperBar = pageOf(qsl("mapper"))->verticalScrollBar();
        QVERIFY2(pMapperBar->maximum() > 0, "the Mapper page fits its viewport, so a scroll position could not be retained or lost");
        const int scrolledTo = pMapperBar->maximum() / 2;
        pMapperBar->setValue(scrolledTo);
        QVERIFY2(scrolledTo > 0, "the Mapper page scrolls by less than two pixels, so this proves nothing");

        selectCategory(qsl("general"));
        QCOMPARE(stack()->currentWidget(), pageOf(qsl("general")));

        selectCategory(qsl("mapper"));
        QCOMPARE(pageOf(qsl("mapper"))->verticalScrollBar()->value(), scrolledTo);
    }

    // A sidebar click gives the list the keyboard focus before the page changes,
    // which is the only reason the case above holds. A deep link arriving while
    // a control on the page has the focus does not, and QStackedLayout then
    // hands that focus over to the incoming page - which, when the control it is
    // taken from has been scrolled out of sight, takes the page back to its top
    // on the way. The control focused here is on the first card, so the page has
    // to scroll to reach it.
    void test_aPageSwitchWithTheFocusOnThePageKeepsItsScrollPosition()
    {
        selectCategory(qsl("mapper"));
        QScrollBar* pMapperBar = pageOf(qsl("mapper"))->verticalScrollBar();
        QVERIFY2(pMapperBar->maximum() > 0, "the Mapper page fits its viewport, so a scroll position could not be retained or lost");
        const int scrolledTo = pMapperBar->maximum() / 2;
        pMapperBar->setValue(scrolledTo);
        QVERIFY2(scrolledTo > 0, "the Mapper page scrolls by less than two pixels, so this proves nothing");

        mpPreferences->pushButton_saveMap->setFocus();
        QCOMPARE(QApplication::focusWidget(), mpPreferences->pushButton_saveMap);
        QCOMPARE(pMapperBar->value(), scrolledTo);

        mpPreferences->setTab(qsl("general"));
        QCoreApplication::processEvents();
        QCOMPARE(stack()->currentWidget(), pageOf(qsl("general")));

        QCOMPARE(pMapperBar->value(), scrolledTo);
        selectCategory(qsl("mapper"));
        QCOMPARE(pageOf(qsl("mapper"))->verticalScrollBar()->value(), scrolledTo);
    }

    // Every change applies itself, so a wheel that happens to pass over a spin
    // box on the way down a page must not be taken for an edit of it
    void test_aWheelOverAnUnfocusedSpinBoxScrollsThePageInstead()
    {
        selectCategory(qsl("mainDisplay"));
        QScrollArea* pPage = pageOf(qsl("mainDisplay"));
        QVERIFY2(pPage->verticalScrollBar()->maximum() > 0, "the Main display page fits its viewport, so there is no scrolling to prefer over an edit");
        pPage->verticalScrollBar()->setValue(0);

        QSpinBox* pSpinBox = mpPreferences->topBorderHeight;
        pSpinBox->setValue(10);
        sidebar()->setFocus();
        QVERIFY2(!pSpinBox->hasFocus(), "the spin box holds the keyboard focus, where a wheel is meant to reach it");

        const QPoint centre = pSpinBox->rect().center();
        QWheelEvent wheel(QPointF(centre), QPointF(pSpinBox->mapToGlobal(centre)), QPoint(0, -40), QPoint(0, -120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(pSpinBox, &wheel);
        QCoreApplication::processEvents();

        QCOMPARE(pSpinBox->value(), 10);
        QVERIFY2(pPage->verticalScrollBar()->value() > 0, "the wheel changed no setting, but the page it was aimed at did not scroll either");
    }

    // The refresh writes its URL and update period into the settings on the way
    // past, so that is what says whether it ran - and it has to run once, not
    // on every walk past the Editor page.
    void test_theEditorThemeRefreshRunsOnTheFirstVisitOnly()
    {
        QSettings* pSettings = mudlet::getQSettings();
        pSettings->remove(qsl("colorSublimeThemesURL"));
        QVERIFY2(!pSettings->contains(qsl("colorSublimeThemesURL")), "the marker this case reads was still set before the first visit");

        selectCategory(qsl("editor"));
        QVERIFY2(pSettings->contains(qsl("colorSublimeThemesURL")), "the first visit to the Editor category did not refresh its themes");
        // ...off the themes file this test wrote, rather than over the network:
        // the label is only shown while a download is running
        QVERIFY2(mpPreferences->theme_download_label->isHidden(), "the Editor category went to the network for its themes instead of reading the file this test wrote");

        pSettings->remove(qsl("colorSublimeThemesURL"));
        selectCategory(qsl("mapper"));
        selectCategory(qsl("editor"));
        QVERIFY2(!pSettings->contains(qsl("colorSublimeThemesURL")), "the Editor category refreshed its themes again on a later visit");
    }

    // retranslateUi() puts back what the .ui file carries and nothing else, so
    // the sidebar, the wordmark, the search field and the cards built in code
    // have to be asked for again or the whole of the navigation stays in the
    // language the dialog was opened in.
    void test_aLanguageChangeRetranslatesTheShell()
    {
        QCOMPARE(sidebar()->item(rowOf(qsl("general")))->text(), qsl("General"));

        BracketingTranslator translator;
        QCoreApplication::installTranslator(&translator);
        mpPreferences->slot_guiLanguageChanged(mudlet::self()->getInterfaceLanguage());
        // Read before the translator goes, so that a failure here cannot leave
        // it installed for the cases that follow
        const QString category = sidebar()->item(rowOf(qsl("general")))->text();
        const QString support = sidebar()->item(sidebar()->count() - 1)->text();
        const QString wordmark = mpPreferences->findChild<QLabel*>(qsl("settingsWordmark"))->text();
        const QString placeholder = mpPreferences->findChild<QLineEdit*>(qsl("settingsSearchField"))->placeholderText();
        const QString cardTitle = mpPreferences->findChild<QGroupBox*>(qsl("card_theme"))->title();
        const QString pageTitle = mpPreferences->findChild<QLabel*>(qsl("settingsPageTitle"))->text();
        QCoreApplication::removeTranslator(&translator);

        QCOMPARE(category, qsl("[General]"));
        QCOMPARE(support, qsl("[Mudlet support]"));
        QCOMPARE(wordmark, qsl("[Settings]"));
        QCOMPARE(placeholder, qsl("[Find in settings]"));
        QCOMPARE(cardTitle, qsl("[Theme]"));
        QCOMPARE(pageTitle, qsl("[General]"));
    }

    // A column narrower than its cards clips them rather than scrolling. This is
    // the invariant behind the width caps, checked over every page rather than
    // the one page a screenshot happens to show.
    void test_noCategoryPageClipsOrSideScrollsItsCards()
    {
        for (const QString& key : specOrderedCategories()) {
            selectCategory(key);
            // the cap is taken again in a deferred turn, once the stylesheet
            // has been applied to the page being shown
            QCoreApplication::processEvents();

            QScrollArea* pPage = pageOf(key);
            QVERIFY2(pPage, qPrintable(qsl("category '%1' has no page").arg(key)));
            QWidget* pColumn = pPage->widget();
            QCOMPARE(pPage->horizontalScrollBar()->maximum(), 0);
            QVERIFY2(pColumn->width() >= pColumn->minimumSizeHint().width(),
                     qPrintable(qsl("the '%1' column is %2px wide but its cards need %3px, so it is clipping them")
                                        .arg(key, QString::number(pColumn->width()), QString::number(pColumn->minimumSizeHint().width()))));
        }
    }

    // The wheel guard covers combo boxes as well as spin boxes, and a combo box
    // is the worse half: several of them apply live, so a wheel passing over
    // one would change the setting there and then.
    void test_aWheelOverAnUnfocusedComboBoxScrollsThePageInstead()
    {
        selectCategory(qsl("mapper"));
        QScrollArea* pPage = pageOf(qsl("mapper"));
        QVERIFY2(pPage->verticalScrollBar()->maximum() > 0, "the Mapper page fits its viewport, so there is no scrolling to prefer over an edit");
        pPage->verticalScrollBar()->setValue(0);

        QComboBox* pComboBox = mpPreferences->comboBox_playerRoomStyle;
        QVERIFY2(pComboBox->count() > 1, "the player room style has one entry, so a wheel could not change it either way");
        pComboBox->setCurrentIndex(0);
        sidebar()->setFocus();
        QVERIFY2(!pComboBox->hasFocus(), "the combo box holds the keyboard focus, where a wheel is meant to reach it");

        const QPoint centre = pComboBox->rect().center();
        QWheelEvent wheel(QPointF(centre), QPointF(pComboBox->mapToGlobal(centre)), QPoint(0, -40), QPoint(0, -120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(pComboBox, &wheel);
        QCoreApplication::processEvents();

        QCOMPARE(pComboBox->currentIndex(), 0);
        QVERIFY2(pPage->verticalScrollBar()->value() > 0, "the wheel changed no setting, but the page it was aimed at did not scroll either");
    }

    // ...and the other half of the same guard: once the control holds the
    // keyboard focus, the wheel is meant for it after all
    void test_aWheelOverAFocusedSpinBoxStillChangesIt()
    {
        selectCategory(qsl("mainDisplay"));
        QSpinBox* pSpinBox = mpPreferences->topBorderHeight;
        pSpinBox->setValue(10);
        pSpinBox->setFocus();
        QVERIFY2(pSpinBox->hasFocus(), "the spin box never took the keyboard focus, so this is the unfocused case again");

        const QPoint centre = pSpinBox->rect().center();
        QWheelEvent wheel(QPointF(centre), QPointF(pSpinBox->mapToGlobal(centre)), QPoint(0, -40), QPoint(0, -120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(pSpinBox, &wheel);
        QCoreApplication::processEvents();

        QVERIFY2(pSpinBox->value() != 10, "the wheel was refused by a spin box that had the keyboard focus");
    }
};

#include "SettingsShellNavigationTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsShellNavigationTest)
