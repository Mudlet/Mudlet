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
 * Run with: ctest -R SettingsShellNavigationTest -V
 */

#include <QDir>
#include <QFileInfo>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QScrollArea>
#include <QSettings>
#include <QScopeGuard>
#include <QSpinBox>
#include <QRegularExpression>
#include <QStackedWidget>
#include <QStyleOptionGroupBox>
#include <QToolButton>
#include <QTranslator>
#include <QWheelEvent>
#include <cmath>

#include "utils.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "SettingsTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"

#include "GroupedTest.h"

// WCAG's ratio, which is what the redesign's contrast targets are written in
static qreal relativeLuminance(const QColor& colour)
{
    const auto channel = [](const qreal value) {
        return value <= 0.04045 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * channel(colour.redF()) + 0.7152 * channel(colour.greenF()) + 0.0722 * channel(colour.blueF());
}

static qreal contrastRatio(const QColor& one, const QColor& other)
{
    const qreal first = relativeLuminance(one);
    const qreal second = relativeLuminance(other);
    return (std::max(first, second) + 0.05) / (std::min(first, second) + 0.05);
}

// What QGroupBox::paintEvent() hands the style, so that a rect asked for here
// is the one the card was actually drawn with - a check indicator is only
// reserved room for when the box is checkable
static QStyleOptionGroupBox groupBoxStyleOption(const QGroupBox* pGroupBox)
{
    QStyleOptionGroupBox option;
    option.initFrom(pGroupBox);
    option.subControls = QStyle::SC_GroupBoxFrame;
    if (pGroupBox->isCheckable()) {
        option.subControls |= QStyle::SC_GroupBoxCheckBox;
        option.state |= pGroupBox->isChecked() ? QStyle::State_On : QStyle::State_Off;
    }
    if (!pGroupBox->title().isEmpty()) {
        option.subControls |= QStyle::SC_GroupBoxLabel;
    }
    option.text = pGroupBox->title();
    option.textAlignment = Qt::AlignLeft;
    option.lineWidth = 0;
    option.midLineWidth = 0;
    return option;
}

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
    QByteArray mSavedNoThemeDownload;
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

    static void deleteProfileDirectory(const QString& profileName) { TestSettings::deleteProfileDirectory(profileName); }

    // The themes the Editor page offers are read from this file; its
    // modification time no longer decides anything, because
    // MUDLET_TEST_NO_THEME_DOWNLOAD is what keeps that page off the network.
    static void writeEditorThemesFile(const QDateTime& modified)
    {
        const QString file = utils::getMudletPath(enums::editorWidgetThemeJsonFile);
        QVERIFY(QDir().mkpath(QFileInfo(file).absolutePath()));
        QFile themes(file);
        QVERIFY(themes.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QVERIFY(themes.write("[]") == 2);
        // Closed before the time is set, because the flush that closing does
        // would otherwise put the modification time back to now
        themes.close();
        QVERIFY(themes.open(QIODevice::ReadWrite));
        QVERIFY(themes.setFileTime(modified, QFileDevice::FileModificationTime));
        themes.close();
    }

    QListWidget* sidebar() const { return TestSettings::sidebar(mpPreferences); }

    QStackedWidget* stack() const { return TestSettings::stack(mpPreferences); }

    QScrollArea* pageOf(const QString& key) const { return TestSettings::pageOf(mpPreferences, key); }

    // Writes through the two references rather than returning them: QVERIFY2
    // expands to a bare return, so anything using it has to return void.
    void scrollMapperPageHalfWayDown(QScrollBar*& pMapperBar, int& scrolledTo)
    {
        pMapperBar = pageOf(qsl("mapper"))->verticalScrollBar();
        QVERIFY2(pMapperBar->maximum() > 0, "the Mapper page fits its viewport, so a scroll position could not be retained or lost");
        scrolledTo = pMapperBar->maximum() / 2;
        pMapperBar->setValue(scrolledTo);
        QVERIFY2(scrolledTo > 0, "the Mapper page scrolls by less than two pixels, so this proves nothing");
    }

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
        // Nothing here may reach github.com for the edbee themes, whatever the
        // themes file on disk looks like
        mSavedNoThemeDownload = qgetenv("MUDLET_TEST_NO_THEME_DOWNLOAD");
        qputenv("MUDLET_TEST_NO_THEME_DOWNLOAD", "1");

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(utils::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);
        // Two days old, so that anything reading the modification time would
        // set off a download - which is exactly what the hook has to stop
        writeEditorThemesFile(QDateTime::currentDateTime().addDays(-2));

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY2(mpHost, "No active host after profile creation");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start()
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

    // The tab widget itself survives, holding no pages, so what is checked is
    // that it is out of the layout rather than deleted
    void test_theTabWidgetIsOutOfTheLayout()
    {
        QCOMPARE(mpPreferences->vBoxLayout_main->indexOf(mpPreferences->tabWidget), -1);
        QCOMPARE(mpPreferences->tabWidget->count(), 0);
        QVERIFY2(mpPreferences->tabWidget->isHidden(), "the emptied tab widget is still on screen");

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

    // The title over a page carries that category's icon beside its name, taken
    // off the sidebar row as the page is shown. The row is handed its icon by
    // the theme pass rather than built with one, and the opening page is shown
    // before that pass runs - so it is the one page whose title can end up bare.
    void test_theOpeningPagesTitleCarriesItsIcon()
    {
        auto* pIcon = mpPreferences->findChild<QLabel*>(qsl("settingsPageTitleIcon"));
        QVERIFY(pIcon);
        QVERIFY2(!pIcon->isHidden(), "the title over the opening page shows no icon at all");
        QVERIFY2(!pIcon->pixmap().isNull(), "the title over the opening page carries a blank icon");
    }

    // Each page is its own scroll area, so walking away from one and coming
    // back has to land where it was left rather than at the top
    void test_eachCategoryKeepsItsOwnScrollPosition()
    {
        selectCategory(qsl("mapper"));
        QScrollBar* pMapperBar = nullptr;
        int scrolledTo = 0;
        scrollMapperPageHalfWayDown(pMapperBar, scrolledTo);

        selectCategory(qsl("general"));
        QCOMPARE(stack()->currentWidget(), pageOf(qsl("general")));

        selectCategory(qsl("mapper"));
        QCOMPARE(pageOf(qsl("mapper"))->verticalScrollBar()->value(), scrolledTo);
    }

    // A sidebar click focuses the list before the page changes, which is why the
    // case above holds. A deep link arriving while a control has the focus does
    // not, and QStackedLayout hands that focus to the incoming page - which takes
    // it back to the top when the control was scrolled out of sight.
    void test_aPageSwitchWithTheFocusOnThePageKeepsItsScrollPosition()
    {
        selectCategory(qsl("mapper"));
        QScrollBar* pMapperBar = nullptr;
        int scrolledTo = 0;
        scrollMapperPageHalfWayDown(pMapperBar, scrolledTo);

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

    // Fusion draws a group box's check indicator from palette(window) darkened
    // by 40%, which on a dark card came out at about 1.1:1 - an outline nobody
    // can see. The shell names that outline itself; this is the measurement
    // that says it is still visible, in the theme it was invisible in.
    void test_aCheckableCardsCheckIndicatorIsVisibleInTheDarkTheme()
    {
        const auto appearanceBefore = mudlet::self()->mAppearance;
        auto restore = qScopeGuard([appearanceBefore]() {
            mudlet::self()->setAppearance(appearanceBefore);
        });
        // Set before the dialog is built, because the shell reads the palette
        // as it assembles its stylesheet
        delete mpPreferences;
        mpPreferences = nullptr;
        mudlet::self()->setAppearance(enums::Appearance::dark);
        openPreferences();
        selectCategory(qsl("privacy"));

        QGroupBox* pCard = mpPreferences->groupBox_ssl;
        QVERIFY2(pCard->isCheckable(), "the card this case measures is not checkable, so it draws no indicator");
        QStyleOptionGroupBox option = groupBoxStyleOption(pCard);
        const QRect indicator = pCard->style()->subControlRect(QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxCheckBox, pCard);
        QVERIFY2(indicator.width() > 4 && indicator.height() > 4, qPrintable(qsl("the check indicator has no rectangle to sample: %1x%2").arg(indicator.width()).arg(indicator.height())));

        const QImage painted = pCard->grab().toImage();
        // The strongest colour the indicator's box is drawn in, against the
        // page showing through beside it
        const QColor page = painted.pixelColor(indicator.right() + 4, indicator.center().y());
        qreal worst = 1.0;
        QColor outline = page;
        for (int x = indicator.left(); x <= indicator.right(); ++x) {
            const QColor sample = painted.pixelColor(x, indicator.top());
            if (contrastRatio(sample, page) > contrastRatio(outline, page)) {
                outline = sample;
            }
        }
        worst = contrastRatio(outline, page);
        QVERIFY2(worst >= 3.0,
                 qPrintable(qsl("the dark theme draws the card's check indicator as %1 against %2, a contrast of %3:1").arg(outline.name(), page.name(), QString::number(worst, 'f', 2))));
    }

    // A checkable card's title starts after its check indicator and a plain
    // one's at the frame edge, which reads as the headings of a page wandering
    // in and out. The plain ones are inset to match rather than the checkable
    // ones being made plain, which landmine 11 forbids.
    void test_everyCardTitleStartsAtTheSameDistanceFromItsFrame()
    {
        selectCategory(qsl("privacy"));

        const auto titleLeft = [](QGroupBox* pCard) {
            QStyleOptionGroupBox option = groupBoxStyleOption(pCard);
            return pCard->style()->subControlRect(QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxLabel, pCard).left();
        };

        QGroupBox* pCheckable = mpPreferences->groupBox_ssl;
        auto* pPlain = mpPreferences->findChild<QGroupBox*>(qsl("card_passwords"));
        QVERIFY(pPlain);
        QVERIFY2(pCheckable->isCheckable(), "the checkable card this case compares against is not checkable");
        QVERIFY2(!pPlain->isCheckable(), "the plain card this case compares is checkable, so there is no inset to check");

        QCOMPARE(titleLeft(pPlain), titleLeft(pCheckable));
    }

    // No test run should depend on a file modification time to stay off the
    // network: a live fetch of github.com fails slowly rather than red
    void test_theThemeDownloadHookKeepsTheEditorPageOffTheNetwork()
    {
        QSettings* pSettings = mudlet::getQSettings();
        const QVariant savedUrl = pSettings->value(qsl("colorSublimeThemesURL"));
        // So that a run where the hook does not hold reaches nothing real
        pSettings->setValue(qsl("colorSublimeThemesURL"), qsl("file:///nonexistent/themes.zip"));

        const QString file = utils::getMudletPath(enums::editorWidgetThemeJsonFile);
        QVERIFY2(QFileInfo(file).lastModified() < QDateTime::currentDateTime().addDays(-1),
                 "the themes file is younger than the update period, so its own freshness would keep this page off the network and the hook would prove nothing");

        selectCategory(qsl("editor"));

        QVERIFY2(mpPreferences->theme_download_label->isHidden(), "a stale themes file still started a theme download, so MUDLET_TEST_NO_THEME_DOWNLOAD did not suppress it");

        savedUrl.isValid() ? pSettings->setValue(qsl("colorSublimeThemesURL"), savedUrl) : pSettings->remove(qsl("colorSublimeThemesURL"));
    }

    // The dialog is a window a player sizes to suit their screen, and one that
    // forgot that between openings would be re-sized on every visit
    void test_theDialogOpensAtTheSizeItWasLastClosedAt()
    {
        // init() opened one at a fixed size; this case wants its own. Bigger
        // than the dialog's 780x560 minimum and smaller than the offscreen
        // platform's 800x600 screen, which restoreGeometry() would shrink a
        // window to fit.
        delete mpPreferences;
        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        mpPreferences->resize(790, 570);
        mpPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpPreferences));
        const QSize chosen = mpPreferences->size();
        QVERIFY2(chosen != QSize(1060, 760), "the size this case chose is the one a dialog with no remembered geometry takes anyway");
        // Closing is what writes the geometry out
        mpPreferences->close();
        delete mpPreferences;

        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        mpPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpPreferences));

        QCOMPARE(mpPreferences->size(), chosen);
    }

    // Every text the search index was built from is replaced by a language
    // change, and the cards it lent out are somewhere else while it happens
    void test_aLanguageChangeMidSearchSendsEveryCardHome()
    {
        selectCategory(qsl("mapper"));
        QLineEdit* pSearchField = mpPreferences->findChild<QLineEdit*>(qsl("settingsSearchField"));
        QVERIFY2(TestSettings::search(mpPreferences, qsl("color")), "the search never ran");
        QVERIFY2(mpPreferences->groupBox_mapperColors->parentWidget() == mpPreferences->findChild<QWidget*>(qsl("settingsColumn_searchResults")),
                 "the search borrowed no card, so sending them home proves nothing");

        BracketingTranslator translator;
        QCoreApplication::installTranslator(&translator);
        mpPreferences->slot_guiLanguageChanged(mudlet::self()->getInterfaceLanguage());
        QCoreApplication::removeTranslator(&translator);
        QCoreApplication::processEvents();

        QVERIFY2(pSearchField->text().isEmpty(), "the language change left the query standing in the search field");
        QCOMPARE(mpPreferences->groupBox_mapperColors->parentWidget(), pageOf(qsl("mapper"))->widget());
        QCOMPARE(stack()->currentWidget(), pageOf(qsl("mapper")));
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

    // A card that holds too much to read on a page of its own is a row that
    // leads to one: the sidebar stays where it was, the title becomes a
    // breadcrumb, and the chevron and Escape both go back up a level.
    void test_aCardWithASubpageDrillsIntoItAndBackOut()
    {
        auto* pSubpage = mpPreferences->findChild<QScrollArea*>(qsl("settingsPage_connection_protocols"));
        auto* pCategoryPage = pageOf(qsl("connection"));
        QVERIFY2(pSubpage, "the game protocols subpage was not built");
        QVERIFY(pCategoryPage);
        selectCategory(qsl("connection"));

        mpPreferences->pushButton_chooseProtocols->click();
        QCoreApplication::processEvents();
        QCOMPARE(stack()->currentWidget(), pSubpage);
        // The sidebar still says which category this page belongs to
        QCOMPARE(sidebar()->currentRow(), rowOf(qsl("connection")));

        auto* pTitle = mpPreferences->findChild<QLabel*>(qsl("settingsPageTitle"));
        const QString breadcrumb = pTitle->text();
        QVERIFY2(breadcrumb.startsWith(sidebar()->item(rowOf(qsl("connection")))->text()), qPrintable(qsl("the subpage's title '%1' does not begin with the category it belongs to").arg(breadcrumb)));
        QVERIFY2(breadcrumb.contains(QChar(0x203a)), qPrintable(qsl("the subpage's title '%1' is not a breadcrumb").arg(breadcrumb)));

        auto* pBack = mpPreferences->findChild<QToolButton*>(qsl("settingsSubpageBack"));
        QVERIFY2(pBack, "there is no subpage back chevron");
        QVERIFY2(pBack->isVisible(), "the subpage showed no back chevron");
        pBack->click();
        QCoreApplication::processEvents();
        QCOMPARE(stack()->currentWidget(), pCategoryPage);
        QCOMPARE(pTitle->text(), sidebar()->item(rowOf(qsl("connection")))->text());
        QVERIFY2(pBack->isHidden(), "the back chevron stayed on show once a category page was back");

        // ...and Escape means the same thing there, rather than closing
        mpPreferences->pushButton_chooseProtocols->click();
        QCoreApplication::processEvents();
        QCOMPARE(stack()->currentWidget(), pSubpage);
        QTest::keyClick(mpPreferences, Qt::Key_Escape);
        QCoreApplication::processEvents();
        QCOMPARE(stack()->currentWidget(), pCategoryPage);
        QVERIFY2(mpPreferences->isVisible(), "Escape on a subpage closed the whole dialog instead of going up a level");
    }

    // Every protocol is a checkbox with a line of its own saying what it does
    void test_everyProtocolIsACheckboxWithADescription()
    {
        const QStringList protocols{qsl("CHARSET"), qsl("GMCP"), qsl("MNES"), qsl("MSDP"), qsl("MSP"), qsl("MSSP"), qsl("MTTS"), qsl("MXP"), qsl("NAWS"), qsl("NEWENVIRON")};
        auto* pSubpage = mpPreferences->findChild<QScrollArea*>(qsl("settingsPage_connection_protocols"));
        QVERIFY2(pSubpage, "the game protocols subpage was not built");
        for (const QString& protocol : protocols) {
            auto* pCheckBox = mpPreferences->findChild<QCheckBox*>(qsl("checkBox_enable%1").arg(protocol));
            QVERIFY2(pCheckBox, qPrintable(qsl("there is no checkbox for the %1 protocol").arg(protocol)));
            QVERIFY2(pSubpage->widget()->isAncestorOf(pCheckBox), qPrintable(qsl("the %1 checkbox is not on the protocols subpage").arg(protocol)));
            QVERIFY2(!pCheckBox->text().isEmpty(), qPrintable(qsl("the %1 checkbox has no label").arg(protocol)));
            auto* pDescription = mpPreferences->findChild<QLabel*>(qsl("checkBox_enable%1_description").arg(protocol));
            QVERIFY2(pDescription, qPrintable(qsl("the %1 checkbox has no description label").arg(protocol)));
            QVERIFY2(!pDescription->text().isEmpty(), qPrintable(qsl("the %1 checkbox's description is empty").arg(protocol)));
        }
    }

    // A description says what a card is for in the player's own terms, and
    // where the wiki has a page about it, ends in a link to that page.
    void test_cardsThatNeedOneCarryADescriptionLine()
    {
        const QStringList described{qsl("groupBox_protocols"), qsl("groupBox_ssl"), qsl("groupBox_proxy"), qsl("card_passwords"), qsl("card_crashReports"), qsl("card_discord")};
        for (const QString& objectName : described) {
            auto* pCard = mpPreferences->findChild<QGroupBox*>(objectName);
            QVERIFY2(pCard, qPrintable(qsl("there is no card called %1").arg(objectName)));
            auto* pDescription = pCard->findChild<QLabel*>(qsl("settingsCardDescription"), Qt::FindDirectChildrenOnly);
            QVERIFY2(pDescription, qPrintable(qsl("the %1 card has no description line").arg(objectName)));
            QVERIFY2(!pDescription->text().isEmpty(), qPrintable(qsl("the %1 card's description is empty").arg(objectName)));
            // The line goes above everything the card already held
            auto* pLayout = pCard->layout();
            QCOMPARE(pLayout->itemAt(0)->widget(), pDescription);
        }

        // Only where a page really exists: every link the descriptions carry
        // has to be one of the wiki pages this phase checked resolves
        const QStringList knownPages{qsl("https://wiki.mudlet.org/w/Manual:Scripting"),
                                     qsl("https://wiki.mudlet.org/w/Manual:Mapper"),
                                     qsl("https://wiki.mudlet.org/w/Standards:Discord_GMCP"),
                                     qsl("https://wiki.mudlet.org/w/Manual:Supported_Protocols"),
                                     qsl("https://wiki.mudlet.org/w/Manual:Unicode"),
                                     qsl("https://wiki.mudlet.org/w/Standards:MUD_Client_Media_Protocol")};
        static const QRegularExpression href(qsl("href=\"([^\"]+)\""));
        int linked = 0;
        for (const auto* pDescription : mpPreferences->findChildren<QLabel*>(qsl("settingsCardDescription"))) {
            for (const auto& match : href.globalMatch(pDescription->text())) {
                ++linked;
                QVERIFY2(knownPages.contains(match.captured(1)), qPrintable(qsl("a card description links to '%1', which is not one of the wiki pages checked for this").arg(match.captured(1))));
            }
        }
        QVERIFY2(linked >= 5, qPrintable(qsl("only %1 card descriptions carry a Learn more link").arg(linked)));
    }

    // The one status hero: it says what the connection is rather than what the
    // settings under it ask for, and it adds no setting of its own.
    void test_theSecurityHeroReportsTheLiveConnection()
    {
        selectCategory(qsl("privacy"));
        auto* pHero = mpPreferences->findChild<QGroupBox*>(qsl("card_securityStatus"));
        QVERIFY2(pHero, "the Privacy and security page has no security status card");
        QScrollArea* pPage = pageOf(qsl("privacy"));
        auto* pColumnLayout = qobject_cast<QBoxLayout*>(pPage->widget()->layout());
        // Above every card on the page, the migration banner aside
        int firstCard = 0;
        while (pColumnLayout->itemAt(firstCard)->widget() && pColumnLayout->itemAt(firstCard)->widget()->objectName() == qsl("settingsMigrationBanner")) {
            ++firstCard;
        }
        QCOMPARE(pColumnLayout->itemAt(firstCard)->widget(), pHero);

        auto* pHeadline = mpPreferences->findChild<QLabel*>(qsl("settingsHeroHeadline"));
        auto* pDetail = mpPreferences->findChild<QLabel*>(qsl("settingsHeroDetail"));
        QVERIFY(pHeadline && pDetail);
        QVERIFY2(!pHeadline->text().isEmpty(), "the security hero says nothing about the connection");
        QVERIFY2(!pDetail->text().isEmpty(), "the security hero offers no detail line");
        // This profile talks to the stub over a plain socket, so whichever way
        // the connection went, the hero must not be claiming encryption
        QVERIFY2(!pHeadline->text().contains(qsl("is encrypted")), qPrintable(qsl("the hero calls an unencrypted connection secure: '%1'").arg(pHeadline->text())));

        // It reflects rather than sets: no control of its own writes a setting
        QVERIFY2(pHero->findChildren<QAbstractButton*>().isEmpty(), "the security hero grew a control of its own - it is meant to reflect and link, not to set");
    }

    // Narrower than a sidebar of names plus a full reading column, the sidebar
    // becomes a rail of icons rather than letting the page scroll sideways
    void test_theSidebarCollapsesToARailWhenTheWindowIsTooNarrowForIt()
    {
        auto* pSidebar = mpPreferences->findChild<QWidget*>(qsl("settingsSidebar"));
        QVERIFY2(pSidebar, "the settings shell has no sidebar to collapse");
        const int fullWidth = pSidebar->width();
        QVERIFY2(fullWidth > 200, qPrintable(qsl("the sidebar is only %1px wide at 1060x760, so this case cannot tell a collapse from where it started").arg(fullWidth)));

        mpPreferences->resize(780, 560);
        QVERIFY2(pSidebar->width() <= 64, qPrintable(qsl("the sidebar is still %1px wide at the dialog's 780x560 minimum").arg(pSidebar->width())));

        // A rail draws no names, but a name is what a screen reader announces
        // the row as - so the item keeps its text, and offers it as a tooltip
        // while there is nowhere on screen to show it
        const int row = rowOf(qsl("mapper"));
        QListWidgetItem* pItem = sidebar()->item(row);
        QVERIFY2(!pItem->text().isEmpty(), "a collapsed sidebar row lost the text that is its accessible name");
        QCOMPARE(pItem->toolTip(), pItem->text());
        // ...and it is still the way to a category
        selectCategory(qsl("mapper"));
        QCOMPARE(stack()->currentWidget(), pageOf(qsl("mapper")));

        mpPreferences->resize(1060, 760);
        QCOMPARE(pSidebar->width(), fullWidth);
        QVERIFY2(sidebar()->item(row)->toolTip().isEmpty(), "an expanded sidebar row kept the tooltip that only stood in for a hidden name");
    }

    // At the size the dialog refuses to go below, no page is wider than the
    // window showing it
    void test_atItsMinimumSizeNoPageScrollsSideways()
    {
        mpPreferences->resize(780, 560);
        qApp->processEvents();
        QStringList pageKeys = specOrderedCategories();
        for (const QString& key : specOrderedCategories()) {
            selectCategory(key);
            // The cap is taken again on the way in, and once more from the
            // event loop after the stylesheet has had its say
            qApp->processEvents();
            QScrollArea* pPage = pageOf(key);
            QVERIFY2(pPage->widget()->width() <= pPage->viewport()->width(),
                     qPrintable(qsl("the '%1' page is %2px wide in a %3px viewport at 780x560").arg(key).arg(pPage->widget()->width()).arg(pPage->viewport()->width())));
            QVERIFY2(!pPage->horizontalScrollBar()->isVisible(), qPrintable(qsl("the '%1' page scrolls sideways at 780x560").arg(key)));
        }
        // ...and the pages the sidebar never selects, which have cards of their
        // own and a breadcrumb over them
        for (const QString& subpage : {qsl("connection/protocols"), qsl("chat/discord")}) {
            mpPreferences->setTab(subpage);
            qApp->processEvents();
            auto* pPage = qobject_cast<QScrollArea*>(stack()->currentWidget());
            QVERIFY2(pPage && pPage->objectName() == qsl("settingsPage_%1").arg(QString(subpage).replace(QLatin1Char('/'), QLatin1Char('_'))),
                     qPrintable(qsl("the deep link '%1' did not reach its subpage").arg(subpage)));
            QVERIFY2(pPage->widget()->width() <= pPage->viewport()->width(),
                     qPrintable(qsl("the '%1' subpage is %2px wide in a %3px viewport at 780x560").arg(subpage).arg(pPage->widget()->width()).arg(pPage->viewport()->width())));
        }
    }

    // A QCheckBox draws its label on one line however long it is, so a
    // translation longer than the reading column makes the page scroll sideways.
    //
    // Not asserted: that English never wraps anything. Whether a label fits
    // depends on this machine's fonts - the same English page wraps under a real
    // X server and not under the offscreen platform - so the claim below is the
    // environment-independent one: only labels that do not fit give up their text.
    void test_aCheckboxTooLongForTheReadingColumnWrapsIntoALabelBesideIt()
    {
        // 137px of English on a 640px column, so no measurement anywhere can
        // ask for this one to be wrapped
        QVERIFY2(!mpPreferences->checkBox_enableBlinkText->text().isEmpty(), "a checkbox that comfortably fits the reading column was wrapped anyway");
        QVERIFY2(!mpPreferences->checkBox_announceIncomingText->text().isEmpty(), "the whole card was wrapped rather than the one label that did not fit it");

        delete mpPreferences;
        mpPreferences = nullptr;
        auto* pGerman = new QTranslator(qApp);
        QVERIFY2(pGerman->load(qsl("mudlet_de_DE"), qsl(":/lang")), "no German translation in the binary's resources, so the wrapping cannot be measured against a real one");
        QVERIFY(qApp->installTranslator(pGerman));
        auto removeTranslator = qScopeGuard([pGerman]() {
            qApp->removeTranslator(pGerman);
            delete pGerman;
        });
        openPreferences();

        QCheckBox* pBox = mpPreferences->checkBox_advertiseScreenReader;
        QVERIFY2(pBox->text().isEmpty(), qPrintable(qsl("the German screen reader checkbox kept its own label: '%1'").arg(pBox->text())));
        QVERIFY2(!pBox->accessibleName().isEmpty(), "the wrapped checkbox has no accessible name, so a screen reader has nothing to announce it as");
        QWidget* pWrap = pBox->parentWidget();
        QCOMPARE(pWrap->objectName(), qsl("settingsCheckBoxWrap"));
        auto* pLabel = pWrap->findChild<QLabel*>(qsl("settingsWrappedLabel"));
        QVERIFY2(pLabel, "the wrapped checkbox has no label beside it to carry its words");
        QCOMPARE(pLabel->text(), pBox->accessibleName());
        QVERIFY2(pLabel->wordWrap(), "the label the checkbox's words moved to does not wrap, which is the whole point of moving them");
        // ...and the page is back inside the reading column because of it
        QCOMPARE(pageOf(qsl("accessibility"))->widget()->maximumWidth(), 640);
        // ...while the checkboxes that fit it are still checkboxes
        QVERIFY2(!mpPreferences->checkBox_enableBlinkText->text().isEmpty(), "a German checkbox that fits the reading column was wrapped with the one that does not");

        // #10165's guarantee: the QCheckBox is still the control of record, so
        // clicking the words it no longer draws has to reach the Host
        selectCategory(qsl("accessibility"));
        qApp->processEvents();
        const bool before = mpHost->mAdvertiseScreenReader;
        QCOMPARE(pBox->isChecked(), before);
        QTest::mouseClick(pLabel, Qt::LeftButton, Qt::NoModifier, pLabel->rect().center());
        QCOMPARE(pBox->isChecked(), !before);
        QCOMPARE(mpHost->mAdvertiseScreenReader, !before);
        mpHost->mAdvertiseScreenReader = before;
    }

    // A card whose title is the name of the page it is alone on tells the reader
    // nothing
    void test_theAccessibilityPageDoesNotRepeatItsOwnNameOnACard()
    {
        selectCategory(qsl("accessibility"));
        const QString pageTitle = mpPreferences->findChild<QLabel*>(qsl("settingsPageTitle"))->text();
        QScrollArea* pPage = pageOf(qsl("accessibility"));
        auto* pColumnLayout = qobject_cast<QBoxLayout*>(pPage->widget()->layout());
        QList<QGroupBox*> cards;
        for (int item = 0, items = pColumnLayout->count(); item < items; ++item) {
            if (auto* pCard = qobject_cast<QGroupBox*>(pColumnLayout->itemAt(item)->widget()); pCard) {
                cards.append(pCard);
            }
        }
        QVERIFY2(cards.size() >= 3, qPrintable(qsl("the Accessibility page carries %1 card(s), so its options were not split up").arg(cards.size())));
        for (auto* pCard : cards) {
            QVERIFY2(!pCard->title().isEmpty(), qPrintable(qsl("the card %1 has no title").arg(pCard->objectName())));
            QVERIFY2(pCard->title() != pageTitle, qPrintable(qsl("the card %1 is called '%2', which is the page's own name").arg(pCard->objectName(), pCard->title())));
        }
        // ...and nothing was lost on the way out of the one card
        const QList<QWidget*> options{mpPreferences->checkBox_announceIncomingText,
                                      mpPreferences->checkBox_advertiseScreenReader,
                                      mpPreferences->checkBox_enableClosedCaption,
                                      mpPreferences->checkBox_enableBlinkText,
                                      mpPreferences->checkBox_f3SearchEnabled,
                                      mpPreferences->comboBox_blankLinesBehaviour,
                                      mpPreferences->comboBox_caretModeKey};
        for (auto* pOption : options) {
            QVERIFY2(pPage->widget()->isAncestorOf(pOption), qPrintable(qsl("%1 fell off the Accessibility page when its card was split").arg(pOption->objectName())));
            QVERIFY2(pOption->isVisible(), qPrintable(qsl("%1 is not showing on the Accessibility page").arg(pOption->objectName())));
        }
    }

    // Escape means "up a level", and search results are a level of their own:
    // starting a search stows the subpage it interrupted. reject() knew nothing
    // of the search, so Escape read the cleared subpage as "nothing to go up
    // from" and closed the dialog (#10241).
    void test_escapeDuringASearchGoesBackToThePageItInterrupted()
    {
        auto* pSearch = mpPreferences->findChild<QLineEdit*>(qsl("settingsSearchField"));
        QVERIFY2(pSearch, "the settings shell has no search field");

        // ...from a category page first
        selectCategory(qsl("mapper"));
        QScrollArea* pMapper = pageOf(qsl("mapper"));
        QCOMPARE(stack()->currentWidget(), pMapper);
        QVERIFY2(TestSettings::search(mpPreferences, qsl("color")), "the search never ran");
        QVERIFY2(stack()->currentWidget() != pMapper, "typing a query never left the category page for the results");

        QTest::keyClick(mpPreferences, Qt::Key_Escape);
        qApp->processEvents();
        QVERIFY2(mpPreferences->isVisible(), "Escape in a search closed the whole dialog instead of leaving the results");
        QVERIFY2(pSearch->text().isEmpty(), "Escape left the query in the field");
        QCOMPARE(stack()->currentWidget(), pMapper);

        // ...and from a subpage, which is the case the search itself promises
        // to come back to
        mpPreferences->setTab(qsl("connection/protocols"));
        qApp->processEvents();
        QScrollArea* pSubpage = pageOf(qsl("connection_protocols"));
        QVERIFY2(pSubpage, "the game protocols subpage was not built");
        QCOMPARE(stack()->currentWidget(), pSubpage);

        QVERIFY2(TestSettings::search(mpPreferences, qsl("color")), "the search never ran");
        QVERIFY2(stack()->currentWidget() != pSubpage, "typing a query never left the subpage for the results");

        QTest::keyClick(mpPreferences, Qt::Key_Escape);
        qApp->processEvents();
        QVERIFY2(mpPreferences->isVisible(), "Escape in a search started on a subpage closed the whole dialog");
        QCOMPARE(stack()->currentWidget(), pSubpage);

        // ...and a second Escape, with no search and no subpage left to leave,
        // still means close
        selectCategory(qsl("general"));
        QTest::keyClick(mpPreferences, Qt::Key_Escape);
        qApp->processEvents();
        QVERIFY2(!mpPreferences->isVisible(), "Escape with nothing left to go up from no longer closes the dialog");
        delete mpPreferences;
        mpPreferences = nullptr;
        openPreferences();
    }

    // A translation can push a page past the reading column, and the window is
    // allowed to be narrower than that page wants. The names in the sidebar are
    // not what pays for it first: they are kept while the page still gets a
    // whole reading column to draw in, and given up only below that. Holding
    // them to the widest page instead left them standing at exactly one window
    // width - see test_narrowingAwayTheWhitespaceKeepsTheSidebarsNames().
    void test_aPageWiderThanTheReadingColumnKeepsOneBeforeTheSidebarGoes()
    {
        delete mpPreferences;
        mpPreferences = nullptr;
        auto* pGerman = new QTranslator(qApp);
        QVERIFY2(pGerman->load(qsl("mudlet_de_DE"), qsl(":/lang")), "no German translation in the binary's resources, so no page is wider than the reading column to measure against");
        QVERIFY(qApp->installTranslator(pGerman));
        auto removeTranslator = qScopeGuard([pGerman]() {
            qApp->removeTranslator(pGerman);
            delete pGerman;
        });
        openPreferences();

        auto* pSidebar = mpPreferences->findChild<QWidget*>(qsl("settingsSidebar"));
        QVERIFY2(pSidebar, "the settings shell has no sidebar to collapse");
        mpPreferences->resize(1500, 760);
        qApp->processEvents();
        const int fullSidebar = pSidebar->width();
        QVERIFY2(fullSidebar > 200, qPrintable(qsl("the sidebar is only %1px wide at 1500x760, so this case cannot tell a collapse from where it started").arg(fullSidebar)));

        // The pages this can be asked about at all: the ones a translation has
        // pushed past the reading column
        QStringList wide;
        for (const QString& key : specOrderedCategories()) {
            selectCategory(key);
            qApp->processEvents();
            if (pageOf(key)->widget()->maximumWidth() > 640) {
                wide.append(key);
            }
        }
        QVERIFY2(!wide.isEmpty(), "no German page is capped wider than the 640px reading column, so the breakpoint cannot be wrong in the way this case describes");

        bool sawACollapse = false;
        for (const QString& key : wide) {
            for (int width = 800; width <= 1400; width += 25) {
                mpPreferences->resize(width, 700);
                qApp->processEvents();
                selectCategory(key);
                qApp->processEvents();
                if (pSidebar->width() < fullSidebar) {
                    sawACollapse = true;
                    continue;
                }
                QScrollArea* pPage = pageOf(key);
                QVERIFY2(pPage->viewport()->width() >= 640,
                         qPrintable(qsl("the German '%1' page was given a %2px viewport - less than the 640px reading column - at a %3px window, while the sidebar kept its full %4px")
                                            .arg(key)
                                            .arg(pPage->viewport()->width())
                                            .arg(width)
                                            .arg(pSidebar->width())));
            }
        }
        QVERIFY2(sawACollapse, "the sidebar kept its names all the way down to an 800px window, so this case never saw the collapse it is about");
        mpPreferences->resize(1060, 760);
    }

    // Clicking the words is how a checkbox is used. The click has to arrive the
    // way the window system delivers one - at the window, for Qt to route - as
    // handing the press straight to the label skips the propagation under test.
    void test_clickingTheWordsOfAWrappedCheckboxTogglesIt()
    {
        delete mpPreferences;
        mpPreferences = nullptr;
        auto* pGerman = new QTranslator(qApp);
        QVERIFY2(pGerman->load(qsl("mudlet_de_DE"), qsl(":/lang")), "no German translation in the binary's resources, so nothing here is wide enough to be wrapped");
        QVERIFY(qApp->installTranslator(pGerman));
        auto removeTranslator = qScopeGuard([pGerman]() {
            qApp->removeTranslator(pGerman);
            delete pGerman;
        });
        openPreferences();

        QLabel* pWrapped = nullptr;
        QCheckBox* pCheckBox = nullptr;
        QString wrappedOn;
        for (const QString& key : specOrderedCategories()) {
            selectCategory(key);
            qApp->processEvents();
            for (QLabel* pLabel : pageOf(key)->widget()->findChildren<QLabel*>(qsl("settingsWrappedLabel"))) {
                auto* pSibling = pLabel->parentWidget()->findChild<QCheckBox*>(QString(), Qt::FindDirectChildrenOnly);
                if (pLabel->isVisible() && pSibling && pSibling->isEnabled()) {
                    pWrapped = pLabel;
                    pCheckBox = pSibling;
                    wrappedOn = key;
                    break;
                }
            }
            if (pWrapped) {
                break;
            }
        }
        QVERIFY2(pWrapped, "no German checkbox was wrapped, so a click on the words it gave up cannot be tested");

        selectCategory(wrappedOn);
        qApp->processEvents();
        pageOf(wrappedOn)->ensureWidgetVisible(pWrapped);
        qApp->processEvents();
        const Qt::CheckState before = pCheckBox->checkState();
        const QPoint inWindow = pWrapped->mapTo(mpPreferences, pWrapped->rect().center());
        QVERIFY2(mpPreferences->childAt(inWindow) == pWrapped, "the point being clicked is not the label, so this would prove nothing about clicking it");
        QTest::mouseClick(mpPreferences->windowHandle(), Qt::LeftButton, Qt::NoModifier, inWindow);
        qApp->processEvents();
        QVERIFY2(pCheckBox->checkState() != before, qPrintable(qsl("clicking the words of the '%1' checkbox left it where it was").arg(pCheckBox->accessibleName())));
    }

    // Every control on a page stops at the reading column, so a window dragged
    // wider than the shell needs would show that width as blank strip beside
    // the settings and nothing else. It refuses to take it.
    void test_theWindowWillNotGrowWiderThanTheShellNeeds()
    {
        // Above the 780x560 minimum and below any width the shell could need,
        // so a window that took this one proves a resize is honoured at all -
        // without which the case below would pass on a window that ignores
        // every resize it is given
        mpPreferences->resize(850, 700);
        qApp->processEvents();
        QCOMPARE(mpPreferences->width(), 850);

        mpPreferences->resize(2000, 700);
        qApp->processEvents();

        // The defect this stops. Measured off the page rather than the stack
        // holding it: the stack fills the content pane whatever the window is
        // doing, and it is the capped page inside it that the strip opens up
        // beside.
        QWidget* pPageShown = stack()->currentWidget();
        const int strip = mpPreferences->width() - pPageShown->mapTo(mpPreferences, QPoint(pPageShown->width(), 0)).x();
        QVERIFY2(strip <= 48, qPrintable(qsl("%1px of the %2px window is empty to the right of the page").arg(strip).arg(mpPreferences->width())));
        QVERIFY2(mpPreferences->width() < 2000, "the window grew to the whole 2000px it was offered");
        QCOMPARE(mpPreferences->width(), mpPreferences->maximumWidth());

        // ...and that it is a width the shell is whole at: the sidebar has its
        // names, and the page is not scrolling sideways to fit
        auto* pSidebar = mpPreferences->findChild<QWidget*>(qsl("settingsSidebar"));
        QVERIFY2(pSidebar && pSidebar->width() > 200, "the widest the window may go is a width the sidebar is still collapsed at");
        QScrollArea* pPage = pageOf(qsl("general"));
        QVERIFY2(pPage->widget()->width() <= pPage->viewport()->width(),
                 qPrintable(qsl("the General page needs %1px and got a %2px viewport at the widest the window may go").arg(pPage->widget()->width()).arg(pPage->viewport()->width())));
    }

    // Reported against the redesign: the strip of empty window beside a page
    // narrower than the widest one could not be taken back, because the first
    // pixel in from the widest the window would go collapsed the sidebar.
    void test_narrowingAwayTheWhitespaceKeepsTheSidebarsNames()
    {
        delete mpPreferences;
        mpPreferences = nullptr;
        // German for the whitespace: the widest the window may go answers to
        // the widest page there is, so a strip only opens up beside the others
        // once a translation has pushed one of them past the reading column
        auto* pGerman = new QTranslator(qApp);
        QVERIFY2(pGerman->load(qsl("mudlet_de_DE"), qsl(":/lang")), "no German translation in the binary's resources, so no page is wider than the reading column to leave a strip beside the rest");
        QVERIFY(qApp->installTranslator(pGerman));
        auto removeTranslator = qScopeGuard([pGerman]() {
            qApp->removeTranslator(pGerman);
            delete pGerman;
        });
        openPreferences();

        auto* pSidebar = mpPreferences->findChild<QWidget*>(qsl("settingsSidebar"));
        QVERIFY2(pSidebar, "the settings shell has no sidebar to collapse");
        mpPreferences->resize(mpPreferences->maximumWidth(), 760);
        qApp->processEvents();
        const int widest = mpPreferences->width();
        QCOMPARE(widest, mpPreferences->maximumWidth());
        const int fullSidebar = pSidebar->width();
        QVERIFY2(fullSidebar > 200, qPrintable(qsl("the sidebar is only %1px wide at the widest the window goes, so this case cannot tell a collapse from where it started").arg(fullSidebar)));

        // What the strip beside a page is: the widest the window may go answers
        // to the widest page there is, and every narrower page is left with the
        // difference. Taken from the caps rather than off the screen, because
        // the gap on screen also holds the pane's margin and the room a page
        // keeps for a scrollbar it is not showing - neither of which is the
        // window's to give back.
        int widestCap = 0;
        QString narrowPage;
        for (const QString& key : specOrderedCategories()) {
            selectCategory(key);
            qApp->processEvents();
            const int cap = pageOf(key)->widget()->maximumWidth();
            widestCap = std::max(widestCap, cap);
            if (cap == 640 && narrowPage.isEmpty()) {
                narrowPage = key;
            }
        }
        QVERIFY2(!narrowPage.isEmpty(), "every German page is capped wider than the reading column, so none of them shows the strip this case is about");
        const int strip = widestCap - 640;
        QVERIFY2(strip > 0, "no German page is capped wider than the reading column, so there is no strip beside the others to take back");

        selectCategory(narrowPage);
        qApp->processEvents();
        mpPreferences->resize(widest - strip, 760);
        qApp->processEvents();
        QCOMPARE(mpPreferences->width(), widest - strip);
        QVERIFY2(pSidebar->width() == fullSidebar,
                 qPrintable(qsl("taking back the %1px the '%2' page is left over - from %3px to %4px - collapsed the sidebar to %5px")
                                    .arg(strip)
                                    .arg(narrowPage)
                                    .arg(widest)
                                    .arg(widest - strip)
                                    .arg(pSidebar->width())));
        mpPreferences->resize(1060, 760);
    }

    // Reported against the redesign: picking a language collapsed the sidebar,
    // on a window nobody had touched. A longer set of translated strings moves
    // the widest the window may go, and the collapse used to be judged against
    // that same number - so the window was suddenly below it.
    void test_aLanguageChangeDoesNotCollapseTheSidebar()
    {
        auto* pSidebar = mpPreferences->findChild<QWidget*>(qsl("settingsSidebar"));
        QVERIFY2(pSidebar, "the settings shell has no sidebar to collapse");
        mpPreferences->resize(mpPreferences->maximumWidth(), 760);
        qApp->processEvents();
        const int widthBefore = mpPreferences->width();
        const int fullSidebar = pSidebar->width();
        QVERIFY2(fullSidebar > 200, qPrintable(qsl("the sidebar is only %1px wide before the language changes, so this case cannot tell a collapse from where it started").arg(fullSidebar)));

        auto* pGerman = new QTranslator(qApp);
        QVERIFY2(pGerman->load(qsl("mudlet_de_DE"), qsl(":/lang")), "no German translation in the binary's resources, so nothing here would lengthen a string");
        QVERIFY(qApp->installTranslator(pGerman));
        mpPreferences->slot_guiLanguageChanged(mudlet::self()->getInterfaceLanguage());
        qApp->processEvents();
        // Read before the translator goes, so a failure cannot leave it
        // installed for the cases that follow
        const int sidebarAfter = pSidebar->width();
        const int widthAfter = mpPreferences->width();
        qApp->removeTranslator(pGerman);
        delete pGerman;

        QCOMPARE(widthAfter, widthBefore);
        QVERIFY2(sidebarAfter == fullSidebar,
                 qPrintable(qsl("changing the language collapsed the sidebar from %1px to %2px, on a window still %3px wide").arg(fullSidebar).arg(sidebarAfter).arg(widthAfter)));
    }
};

#include "SettingsShellNavigationTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsShellNavigationTest)
