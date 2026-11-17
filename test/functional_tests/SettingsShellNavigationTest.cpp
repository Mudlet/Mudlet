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
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSettings>
#include <QScopeGuard>
#include <QSpinBox>
#include <QRegularExpression>
#include <QStackedWidget>
#include <QStyleOptionButton>
#include <QStyleOptionGroupBox>
#include <QToolButton>
#include <QTranslator>
#include <QWheelEvent>
#include <cmath>

#include "MudletApp.h"
#include "PixelTestHelper.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "SettingsTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"
#include "uiDesign.h"

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

// Summed over three channels: a border over a rounded corner is antialiased
// against what is behind it, so nearness is measured rather than a match
static constexpr int scmHairlineInkSlack = 24;
// Past this a pixel in a mark's box is part of the mark, not the fill's noise
static constexpr int scmMarkInkSlack = 40;

static QRect inTheGrabsPixels(const QImage& shot, const QRect& box)
{
    const qreal ratio = shot.devicePixelRatio();
    return QRect(QPoint(qRound(box.left() * ratio), qRound(box.top() * ratio)), QPoint(qRound((box.right() + 1) * ratio) - 1, qRound((box.bottom() + 1) * ratio) - 1)).intersected(shot.rect());
}

// Read off the window, not the control: a control with no background is grabbed
// against its own palette, so the card showing through is only in the window's grab
static QRect markBoxIn(const QAbstractButton* pButton, const QWidget* pWindow, const QImage& shot)
{
    QStyleOptionButton option;
    option.initFrom(pButton);
    const QStyle::SubElement part = qobject_cast<const QRadioButton*>(pButton) ? QStyle::SE_RadioButtonIndicator : QStyle::SE_CheckBoxIndicator;
    QRect box = pButton->style()->subElementRect(part, &option, pButton);
    box.moveTopLeft(pButton->mapTo(pWindow, box.topLeft()));
    return inTheGrabsPixels(shot, box);
}

static qreal strongestOutlineRatio(const QImage& shot, const QRect& box, const QColor& behind)
{
    QColor outline = behind;
    for (int x = box.left(); x <= box.right(); ++x) {
        const QColor sample = shot.pixelColor(x, box.top());
        if (contrastRatio(sample, behind) > contrastRatio(outline, behind)) {
            outline = sample;
        }
    }
    return contrastRatio(outline, behind);
}

static int markedPixelsIn(const QImage& shot, const QRect& box, const QColor& fill)
{
    const QRect inside = box.adjusted(box.width() / 4, box.height() / 4, -box.width() / 4, -box.height() / 4);
    int marked = 0;
    for (int y = inside.top(); y <= inside.bottom(); ++y) {
        for (int x = inside.left(); x <= inside.right(); ++x) {
            const QColor sample = shot.pixelColor(x, y);
            if (TestPixels::distance(sample, fill) > scmMarkInkSlack) {
                ++marked;
            }
        }
    }
    return marked;
}

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
        const QString file = MudletApp::getMudletPath(enums::editorWidgetThemeJsonFile);
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

    // macOS's interface font; elsewhere the family falls back to the platform's own, no narrower
    static QFont interfaceFontAt13pt()
    {
        QFont font(qsl(".AppleSystemUIFont"));
        font.setPointSize(13);
        return font;
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

    // Set rather than clicked: one of the three states is one no click can reach
    QImage grabAfterSetting(QCheckBox* pBox, const Qt::CheckState state) const
    {
        pBox->setCheckState(state);
        QCoreApplication::processEvents();
        return mpPreferences->grab().toImage();
    }

    // Small steps so no width in between is skipped. The names never fold for less
    // than the reading column, or a page switch would fold them for nothing.
    void walkTheWindowDown()
    {
        auto* pSidebar = mpPreferences->findChild<QWidget*>(qsl("settingsSidebar"));
        auto* pContent = mpPreferences->findChild<QWidget*>(qsl("settingsContent"));
        QVERIFY2(pSidebar && pContent && pContent->layout(), "the settings shell has no sidebar or no content pane");
        // Well past the width the names and the widest page's column need, as the window has no widest
        mpPreferences->resize(mpPreferences->sidebarWidths().fullyExpanded + 400, 760);
        qApp->processEvents();
        const int widest = mpPreferences->width();
        const int fullSidebar = pSidebar->width();
        QVERIFY2(fullSidebar > 100, qPrintable(qsl("the sidebar is only %1px wide at %2px, so this case cannot tell a fold from where it started").arg(fullSidebar).arg(widest)));

        int widestPageMinimum = 0;
        QString widestPage;
        for (int index = 0, pages = stack()->count(); index < pages; ++index) {
            auto* pPage = qobject_cast<QScrollArea*>(stack()->widget(index));
            if (!pPage || !pPage->widget() || pPage->widget()->minimumSizeHint().width() <= widestPageMinimum) {
                continue;
            }
            widestPageMinimum = pPage->widget()->minimumSizeHint().width();
            // As setTab() takes it, "category" or "category/subpage"; the search results are reached by a search
            if (const QString key = pPage->objectName().section(QLatin1Char('_'), 1); key != qsl("searchResults")) {
                widestPage = key.contains(QLatin1Char('_')) ? qsl("%1/%2").arg(key.section(QLatin1Char('_'), 0, 0), key.section(QLatin1Char('_'), 1)) : key;
            }
        }
        QVERIFY2(!widestPage.isEmpty(), "no settings page has a column to measure");
        const QMargins margins = pContent->layout()->contentsMargins();
        const int sidebarAndMargins = fullSidebar + margins.left() + margins.right();
        const int scrollBar = pageOf(qsl("general"))->verticalScrollBar()->sizeHint().width();

        QStringList walked{qsl("editor")};
        if (widestPage != walked.constFirst()) {
            walked.append(widestPage);
        }
        for (const QString& page : std::as_const(walked)) {
            mpPreferences->resize(widest, 760);
            qApp->processEvents();
            mpPreferences->setTab(page);
            qApp->processEvents();
            auto* pPage = qobject_cast<QScrollArea*>(stack()->currentWidget());
            QVERIFY2(pPage && pPage->widget(), qPrintable(qsl("the '%1' page did not come up").arg(page)));
            QWidget* pColumn = pPage->widget();
            const int threshold = sidebarAndMargins + std::max(std::min(640, widestPageMinimum), pColumn->minimumSizeHint().width()) + scrollBar;
            const int narrowest = mpPreferences->minimumWidth();
            bool sawNames = false;
            bool sawRail = false;
            for (int width = widest; width > narrowest - 8; width -= 8) {
                mpPreferences->resize(std::max(width, narrowest), 760);
                qApp->processEvents();
                // The resize posts the scroll area's own layout pass, which hides a bar the new width no longer needs
                QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
                qApp->processEvents();
                const int actual = mpPreferences->width();
                QVERIFY2(!pPage->horizontalScrollBar()->isVisible(),
                         qPrintable(qsl("the '%1' page scrolls sideways in a %2px window, with the sidebar %3px wide").arg(page).arg(actual).arg(pSidebar->width())));
                QVERIFY2(pColumn->geometry().right() < pPage->viewport()->width(),
                         qPrintable(qsl("the '%1' page's cards end at %2px in a %3px viewport, in a %4px window with the sidebar %5px wide")
                                            .arg(page)
                                            .arg(pColumn->geometry().right() + 1)
                                            .arg(pPage->viewport()->width())
                                            .arg(actual)
                                            .arg(pSidebar->width())));
                const bool rail = sidebar()->property(uiDesign::scmProp_rail).toBool();
                if (actual < threshold) {
                    QVERIFY2(rail,
                             qPrintable(qsl("the sidebar kept its names in a %1px window on the '%2' page, %3px short of the %4px they need beside it")
                                                .arg(actual)
                                                .arg(page)
                                                .arg(threshold - actual)
                                                .arg(threshold)));
                } else {
                    QVERIFY2(!rail, qPrintable(qsl("the sidebar folded in a %1px window on the '%2' page, which holds the %3px its names need beside it").arg(actual).arg(page).arg(threshold)));
                }
                sawNames |= !rail;
                sawRail |= rail;
            }
            QVERIFY2(sawNames && sawRail, qPrintable(qsl("walking the '%1' page from %2px down to %3px never saw both the names and the rail").arg(page).arg(widest).arg(narrowest)));
        }
        mpPreferences->resize(1060, 760);
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
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mProfileName);
        // Two days old, so that anything reading the modification time would
        // set off a download - which is exactly what the hook has to stop
        writeEditorThemesFile(QDateTime::currentDateTime().addDays(-2));

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY2(mpHost, "No active host after profile creation");
        // Set outright: this file tests what width does to a sidebar asked for its
        // names, and the default is SettingsSidebarToggleTest's
        MudletApp::getQSettings()->setValue(qsl("settingsSidebarLabelsShown"), true);
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
    // Set before the dialog is built, as the platform does: macOS gives a list a smaller font than the window's, and a
    // width measured before the list took it kept the sidebar wider until the next restyle
    // A wrapping card measures its minimum height at the column's narrowest width, many lines taller than at the
    // width it is given, and the page kept its scroll bar until the window reached that height
    void test_aPageWhoseCardsFitShowsNoScrollBar()
    {
        QScrollArea* pPage = pageOf(qsl("general"));
        QWidget* pColumn = pPage->widget();
        const auto settle = [this](const int height) {
            mpPreferences->resize(mpPreferences->width(), height);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
            QCoreApplication::processEvents();
        };
        const auto cardsHeight = [pPage, pColumn]() {
            return pColumn->heightForWidth(pPage->maximumViewportSize().width());
        };
        QVERIFY2(pColumn->layout()->minimumSize().height() > cardsHeight(),
                 qPrintable(qsl("SETUP: the cards' minimum heights add up to %1px, no more than the %2px they take, so nothing wraps taller at the narrowest")
                                    .arg(pColumn->layout()->minimumSize().height())
                                    .arg(cardsHeight())));

        const int chrome = mpPreferences->height() - pPage->maximumViewportSize().height();
        settle(chrome + cardsHeight() + 8);
        QTRY_VERIFY2(!pPage->verticalScrollBar()->isVisible(),
                     qPrintable(qsl("the page shows a scroll bar with %1px of room for %2px of cards").arg(pPage->maximumViewportSize().height()).arg(cardsHeight())));

        settle(chrome + cardsHeight() - 40);
        QTRY_VERIFY2(pPage->verticalScrollBar()->isVisible(), "SETUP: the page shows no scroll bar even when its cards do not fit");
    }

    // The other side of the case above: a page whose cards do not fit keeps its scroll bar and its cards' heights,
    // whether or not anything on it wraps
    void test_aPageWhoseCardsDoNotFitKeepsItsScrollBar()
    {
        mpPreferences->resize(mpPreferences->width(), mpPreferences->minimumHeight());
        QStringList checked;
        for (int index = 0, pages = stack()->count(); index < pages; ++index) {
            auto* pPage = qobject_cast<QScrollArea*>(stack()->widget(index));
            if (!pPage) {
                continue;
            }
            stack()->setCurrentWidget(pPage);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
            QCoreApplication::processEvents();
            QWidget* pColumn = pPage->widget();
            const bool wraps = pColumn->layout()->hasHeightForWidth();
            const int needed = wraps ? pColumn->heightForWidth(pColumn->width()) : pColumn->layout()->minimumSize().height();
            if (needed <= pPage->viewport()->height()) {
                continue;
            }
            checked << qsl("%1%2").arg(pPage->objectName(), wraps ? qsl(" (wraps)") : QString());
            QVERIFY2(pPage->verticalScrollBar()->isVisible(),
                     qPrintable(qsl("'%1' shows no scroll bar for %2px of cards in %3px").arg(pPage->objectName()).arg(needed).arg(pPage->viewport()->height())));
            QVERIFY2(pColumn->height() >= needed, qPrintable(qsl("'%1' squeezes %2px of cards into %3px").arg(pPage->objectName()).arg(needed).arg(pColumn->height())));
        }
        qDebug() << "pages too tall for the window:" << checked;
        QVERIFY2(checked.size() > 1, "SETUP: at most one page is taller than the shortest window, so this checks next to nothing");
    }

    void test_theSidebarOpensAtTheWidthARestyleKeeps()
    {
        const QFont savedListFont = QApplication::font("QAbstractItemView");
        auto restore = qScopeGuard([&savedListFont]() {
            QApplication::setFont(savedListFont, "QAbstractItemView");
        });
        QFont listFont = QApplication::font();
        listFont.setPointSizeF(QApplication::font().pointSizeF() + 3);
        QApplication::setFont(listFont, "QAbstractItemView");

        auto* pDialog = new dlgProfilePreferences(mudlet::self(), mpHost);
        auto discard = qScopeGuard([pDialog]() {
            delete pDialog;
        });
        pDialog->resize(1060, 760);
        pDialog->show();
        QVERIFY(QTest::qWaitForWindowExposed(pDialog));
        // The platform's own list font wins over the one set above where it has one, as on macOS, which has the same effect
        QVERIFY2(pDialog->mpListWidget_categories->font().pointSizeF() != pDialog->font().pointSizeF(),
                 "SETUP: the sidebar's list has the window's font, so there is no later font for a measurement to miss");
        const int opened = pDialog->measuredSidebarWidth();

        pDialog->applyShellStyle();
        QCoreApplication::processEvents();
        const int restyled = pDialog->measuredSidebarWidth();
        QVERIFY2(opened == restyled,
                 qPrintable(qsl("the sidebar opened %1px wide and a restyle made it %2px, so it was measured before its rows had the font they are drawn in")
                                    .arg(QString::number(opened), QString::number(restyled))));
    }

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
        QSettings* pSettings = MudletApp::getQSettings();
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
        // In the grab's pixels: on a doubling screen the card's coordinates sample the
        // padding above and left of the indicator and read 1:1 whatever it is drawn as
        const QRect drawn = inTheGrabsPixels(painted, indicator);
        // The strongest colour the indicator's box is drawn in, against the
        // page showing through beside it
        const QColor page = painted.pixelColor(qMin(drawn.right() + 4, painted.width() - 1), drawn.center().y());
        qreal worst = 1.0;
        QColor outline = page;
        for (int x = drawn.left(); x <= drawn.right(); ++x) {
            const QColor sample = painted.pixelColor(x, drawn.top());
            if (contrastRatio(sample, page) > contrastRatio(outline, page)) {
                outline = sample;
            }
        }
        worst = contrastRatio(outline, page);
        QVERIFY2(worst >= 3.0,
                 qPrintable(qsl("the dark theme draws the card's check indicator as %1 against %2, a contrast of %3:1").arg(outline.name(), page.name(), QString::number(worst, 'f', 2))));
    }

    void test_everyCardsFirstLineStartsAtTheSameDistanceFromItsFrame()
    {
        selectCategory(qsl("privacy"));

        const auto firstLineLeft = [](QGroupBox* pCard) {
            QStyleOptionGroupBox option = groupBoxStyleOption(pCard);
            const QStyle::SubControl leading = pCard->isCheckable() ? QStyle::SC_GroupBoxCheckBox : QStyle::SC_GroupBoxLabel;
            const QRect frame = pCard->style()->subControlRect(QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxFrame, pCard);
            return pCard->style()->subControlRect(QStyle::CC_GroupBox, &option, leading, pCard).left() - frame.left();
        };

        QGroupBox* pCheckable = mpPreferences->groupBox_ssl;
        auto* pPlain = mpPreferences->findChild<QGroupBox*>(qsl("card_passwords"));
        QVERIFY(pPlain);
        QVERIFY2(pCheckable->isCheckable(), "the checkable card this case compares against is not checkable");
        QVERIFY2(!pPlain->isCheckable(), "the plain card this case compares is checkable, so the two lead with the same thing");

        QCOMPARE(firstLineLeft(pPlain), firstLineLeft(pCheckable));
    }

    // No stylesheet reserves room for a title inside the frame: the card's top
    // padding alone holds the first control clear of it
    void test_aCardsTitleIsDrawnInsideTheFrameWithItsControlsClearOfIt()
    {
        selectCategory(qsl("privacy"));

        QGroupBox* pCheckable = mpPreferences->groupBox_ssl;
        auto* pPlain = mpPreferences->findChild<QGroupBox*>(qsl("card_passwords"));
        QVERIFY(pPlain);
        QVERIFY2(pCheckable->isCheckable(), "the checkable card this case measures is not checkable, so its indicator is not being placed");

        for (QGroupBox* pCard : {pCheckable, pPlain}) {
            QStyleOptionGroupBox option = groupBoxStyleOption(pCard);
            const QRect frame = pCard->style()->subControlRect(QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxFrame, pCard);
            const QRect title = pCard->style()->subControlRect(QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxLabel, pCard);
            const QRect contents = pCard->style()->subControlRect(QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxContents, pCard);
            const QString card = pCard->objectName();
            const auto asText = [](const QRect& rect) {
                return qsl("%1x%2+%3+%4").arg(rect.width()).arg(rect.height()).arg(rect.left()).arg(rect.top());
            };
            QVERIFY2(frame.contains(title), qPrintable(qsl("the card %1 draws its title at %2, which is not inside its frame at %3").arg(card, asText(title), asText(frame))));
            QVERIFY2(contents.top() >= title.bottom(),
                     qPrintable(qsl("the card %1 starts its controls at y=%2, on top of a title that runs to y=%3").arg(card).arg(contents.top()).arg(title.bottom())));
            if (pCard->isCheckable()) {
                const QRect indicator = pCard->style()->subControlRect(QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxCheckBox, pCard);
                QVERIFY2(frame.contains(indicator), qPrintable(qsl("the card %1 draws its check indicator at %2, which is not inside its frame at %3").arg(card, asText(indicator), asText(frame))));
                QVERIFY2(indicator.right() <= title.left(),
                         qPrintable(qsl("the card %1 draws its check indicator over its title: the indicator ends at x=%2 and the title starts at x=%3")
                                            .arg(card)
                                            .arg(indicator.right())
                                            .arg(title.left())));
                // Half a line at most: further off, the title band's room was guessed, not measured
                QVERIFY2(std::abs(indicator.center().y() - title.center().y()) <= title.height() / 2,
                         qPrintable(qsl("the card %1 centres its check indicator at y=%2 and its title at y=%3").arg(card).arg(indicator.center().y()).arg(title.center().y())));
            }
        }
    }

    // Both appearances: unstyled, the platform draws a grey Fusion square on one, a
    // blue rounded box on the other, and Qt::PartiallyChecked as a filled grey square
    void test_everyChoiceOnAPageCarriesTheOneMark()
    {
        const auto appearanceBefore = mudlet::self()->mAppearance;
        auto restore = qScopeGuard([appearanceBefore]() {
            mudlet::self()->setAppearance(appearanceBefore);
        });

        QStringList misses;
        for (const auto& appearance : QList<QPair<QString, enums::Appearance>>{{qsl("dark"), enums::Appearance::dark}, {qsl("light"), enums::Appearance::light}}) {
            // Before the dialog is built: the shell reads the palette as it assembles its stylesheet
            delete mpPreferences;
            mpPreferences = nullptr;
            mudlet::self()->setAppearance(appearance.second);
            openPreferences();

            selectCategory(qsl("general"));
            QCheckBox* pBox = mpPreferences->mFORCE_SAVE_ON_EXIT;
            QVERIFY2(pBox->isVisible(), "the check box this case measures is not on show");
            const Qt::CheckState boxWas = pBox->checkState();
            const bool wasTristate = pBox->isTristate();
            auto putTheBoxBack = qScopeGuard([pBox, boxWas, wasTristate]() {
                pBox->setTristate(wasTristate);
                pBox->setCheckState(boxWas);
            });
            pBox->setTristate(true);

            const QImage empty = grabAfterSetting(pBox, Qt::Unchecked);
            const QRect emptyBox = markBoxIn(pBox, mpPreferences, empty);
            QVERIFY2(emptyBox.width() > 4 && emptyBox.height() > 4,
                     qPrintable(qsl("%1: the check box's mark has no rectangle to sample: %2x%3").arg(appearance.first).arg(emptyBox.width()).arg(emptyBox.height())));

            // Beside the mark: the outline is read against the card showing through
            const QColor behind = empty.pixelColor(qMin(emptyBox.right() + emptyBox.width() / 3, empty.width() - 1), emptyBox.center().y());
            const qreal outline = strongestOutlineRatio(empty, emptyBox, behind);
            if (outline < uiDesign::scmQuietMinimumRatio) {
                misses << qsl("%1: a check box outlines its mark at %2:1 against the card behind it").arg(appearance.first, QString::number(outline, 'f', 2));
            }

            const QColor fill = empty.pixelColor(emptyBox.center());
            const int nothingInIt = markedPixelsIn(empty, emptyBox, fill);
            const int aTick = markedPixelsIn(grabAfterSetting(pBox, Qt::Checked), emptyBox, fill);
            const int aDash = markedPixelsIn(grabAfterSetting(pBox, Qt::PartiallyChecked), emptyBox, fill);
            qInfo().noquote() << qsl("  %1: the check box's mark is %2px, outlined at %3:1 on a fill of %4, with %5 pixels in it empty, %6 checked and %7 part-checked")
                                         .arg(appearance.first)
                                         .arg(emptyBox.width())
                                         .arg(QString::number(outline, 'f', 2), fill.name())
                                         .arg(nothingInIt)
                                         .arg(aTick)
                                         .arg(aDash);
            if (nothingInIt > 0) {
                misses << qsl("%1: an unchecked box already has %2 pixels in it that are not its fill").arg(appearance.first).arg(nothingInIt);
            }
            if (aTick <= 0) {
                misses << qsl("%1: a checked box paints nothing inside itself, so there is no tick").arg(appearance.first);
            }
            if (aDash <= 0) {
                misses << qsl("%1: a Qt::PartiallyChecked box paints nothing inside itself, so it reads as an empty one").arg(appearance.first);
            }

            selectCategory(qsl("inputLine"));
            QRadioButton* pChosen = mpPreferences->radioButton_userDictionary_profile;
            QRadioButton* pOther = mpPreferences->radioButton_userDictionary_common;
            QVERIFY2(pChosen->isVisible(), "the radio buttons this case measures are not on show");
            const bool profileWas = pChosen->isChecked();
            auto putTheRadioBack = qScopeGuard([pChosen, pOther, profileWas]() {
                (profileWas ? pChosen : pOther)->setChecked(true);
            });

            pOther->setChecked(true);
            QCoreApplication::processEvents();
            const QImage unchosen = mpPreferences->grab().toImage();
            const QRect radioBox = markBoxIn(pChosen, mpPreferences, unchosen);
            QVERIFY2(radioBox.width() > 4 && radioBox.height() > 4,
                     qPrintable(qsl("%1: the radio button's mark has no rectangle to sample: %2x%3").arg(appearance.first).arg(radioBox.width()).arg(radioBox.height())));
            const QColor radioBehind = unchosen.pixelColor(qMin(radioBox.right() + radioBox.width() / 3, unchosen.width() - 1), radioBox.center().y());
            const qreal radioOutline = strongestOutlineRatio(unchosen, radioBox, radioBehind);
            const QColor radioFill = unchosen.pixelColor(radioBox.center());
            const int noDot = markedPixelsIn(unchosen, radioBox, radioFill);

            pChosen->setChecked(true);
            QCoreApplication::processEvents();
            const int aDot = markedPixelsIn(mpPreferences->grab().toImage(), radioBox, radioFill);
            qInfo().noquote() << qsl("  %1: the radio button's mark is %2px, outlined at %3:1 on a fill of %4, with %5 pixels in it unchosen and %6 chosen")
                                         .arg(appearance.first)
                                         .arg(radioBox.width())
                                         .arg(QString::number(radioOutline, 'f', 2), radioFill.name())
                                         .arg(noDot)
                                         .arg(aDot);
            if (radioOutline < uiDesign::scmQuietMinimumRatio) {
                misses << qsl("%1: a radio button outlines its mark at %2:1 against the card behind it").arg(appearance.first, QString::number(radioOutline, 'f', 2));
            }
            if (aDot <= noDot) {
                misses << qsl("%1: a chosen radio button paints %2 pixels inside itself against the %3 an unchosen one does, so there is no dot").arg(appearance.first).arg(aDot).arg(noDot);
            }
        }
        QVERIFY2(misses.isEmpty(), qPrintable(qsl("\n  %1").arg(misses.join(qsl("\n  ")))));
    }

    void test_aButtonOnAPageIsFramedInTheDesignsHairline()
    {
        const auto appearanceBefore = mudlet::self()->mAppearance;
        auto restore = qScopeGuard([appearanceBefore]() {
            mudlet::self()->setAppearance(appearanceBefore);
        });

        QStringList misses;
        for (const auto& appearance : QList<QPair<QString, enums::Appearance>>{{qsl("dark"), enums::Appearance::dark}, {qsl("light"), enums::Appearance::light}}) {
            delete mpPreferences;
            mpPreferences = nullptr;
            mudlet::self()->setAppearance(appearance.second);
            openPreferences();
            selectCategory(qsl("general"));

            QPushButton* pButton = mpPreferences->pushButton_whereToLog;
            QVERIFY2(pButton->isVisible(), "the button this case measures is not on show");
            const QColor border = uiDesign::themeTokens().border;
            const QImage shot = pButton->grab().toImage();
            const int ratio = qMax(1, qRound(shot.devicePixelRatio()));

            // Mid top edge, a straight run rather than the rounded corner. A doubling
            // screen draws the hairline over two rows, so the nearer one is read.
            int nearest = 3 * 255;
            QColor read = border;
            for (int row = 0; row < ratio; ++row) {
                const QColor sample = shot.pixelColor(shot.width() / 2, row);
                const int distance = TestPixels::distance(sample, border);
                if (distance < nearest) {
                    nearest = distance;
                    read = sample;
                }
            }
            qInfo().noquote() << qsl("  %1: the button's top edge reads %2 against a hairline of %3, %4 away").arg(appearance.first).arg(read.name(), border.name()).arg(nearest);
            if (nearest > scmHairlineInkSlack) {
                misses << qsl("%1: the button's outline reads %2 rather than the design's %3").arg(appearance.first, read.name(), border.name());
            }
        }
        QVERIFY2(misses.isEmpty(), qPrintable(qsl("\n  %1").arg(misses.join(qsl("\n  ")))));
    }

    // QAbstractButton reads QStyle::SH_Button_FocusPolicy once, in its constructor,
    // and the base style swaps with the appearance (DarkTheme's Fusion, the
    // platform's own), so the dialog is rebuilt under each appearance
    void test_aClickOnAChoiceOrAButtonLeavesTheFocusToTheKeyboard()
    {
        const auto appearanceBefore = mudlet::self()->mAppearance;
        auto restore = qScopeGuard([appearanceBefore]() {
            mudlet::self()->setAppearance(appearanceBefore);
        });

        QStringList misses;
        for (const auto& appearance : QList<QPair<QString, enums::Appearance>>{{qsl("dark"), enums::Appearance::dark}, {qsl("light"), enums::Appearance::light}}) {
            delete mpPreferences;
            mpPreferences = nullptr;
            mudlet::self()->setAppearance(appearance.second);
            openPreferences();
            selectCategory(qsl("general"));

            QCheckBox* pBox = mpPreferences->mFORCE_SAVE_ON_EXIT;
            QVERIFY2(pBox->isVisible(), "the check box this case clicks is not on show");
            const Qt::CheckState boxWas = pBox->checkState();
            auto putTheBoxBack = qScopeGuard([pBox, boxWas]() {
                pBox->setCheckState(boxWas);
                pBox->clearFocus();
            });

            // On the mark: a check box whose words moved to a label beside it answers
            // a click over its indicator and nothing else
            QStyleOptionButton option;
            option.initFrom(pBox);
            QTest::mouseClick(pBox, Qt::LeftButton, Qt::NoModifier, pBox->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option, pBox).center());
            qApp->processEvents();
            if (pBox->checkState() == boxWas) {
                misses << qsl("%1: clicking the check box left it where it was, so nothing here says the click landed").arg(appearance.first);
                continue;
            }
            // focusWidget(), not hasFocus(): that reads false when the window never
            // became active, and would pass whatever the focus policy said
            if (mpPreferences->focusWidget() == pBox) {
                misses << qsl("%1: a click left the check box holding the focus, so the accent stayed on its mark").arg(appearance.first);
            }
            if (pBox->hasFocus()) {
                misses << qsl("%1: a click left the check box focused").arg(appearance.first);
            }

            pBox->setFocus(Qt::TabFocusReason);
            qApp->processEvents();
            if (mpPreferences->focusWidget() != pBox) {
                misses << qsl("%1: the Tab key cannot reach the check box, so a keyboard user has nothing to see the accent on").arg(appearance.first);
            }

            pBox->clearFocus();

            // Signals blocked: this button opens a file dialog, and what is measured
            // happens in QApplication's delivery of the press, not in the slot
            QPushButton* pButton = mpPreferences->pushButton_whereToLog;
            QVERIFY2(pButton->isVisible(), "the button this case clicks is not on show");
            const QSignalBlocker noFileDialog(pButton);
            QTest::mouseClick(pButton, Qt::LeftButton, Qt::NoModifier, pButton->rect().center());
            qApp->processEvents();
            qInfo().noquote() << qsl("  %1: the base style is %2, which asks for focus policy %3; the check box carries %4 and the button %5")
                                         .arg(appearance.first, qApp->style()->objectName())
                                         .arg(qApp->style()->styleHint(QStyle::SH_Button_FocusPolicy))
                                         .arg(static_cast<int>(pBox->focusPolicy()))
                                         .arg(static_cast<int>(pButton->focusPolicy()));
            if (mpPreferences->focusWidget() == pButton) {
                misses << qsl("%1: a click left the button holding the focus, so the accent stayed round it").arg(appearance.first);
            }
            if (pButton->hasFocus()) {
                misses << qsl("%1: a click left the button focused").arg(appearance.first);
            }
        }
        QVERIFY2(misses.isEmpty(), qPrintable(qsl("\n  %1").arg(misses.join(qsl("\n  ")))));
    }

    // No test run should depend on a file modification time to stay off the
    // network: a live fetch of github.com fails slowly rather than red
    void test_theThemeDownloadHookKeepsTheEditorPageOffTheNetwork()
    {
        QSettings* pSettings = MudletApp::getQSettings();
        const QVariant savedUrl = pSettings->value(qsl("colorSublimeThemesURL"));
        // So that a run where the hook does not hold reaches nothing real
        pSettings->setValue(qsl("colorSublimeThemesURL"), qsl("file:///nonexistent/themes.zip"));

        const QString file = MudletApp::getMudletPath(enums::editorWidgetThemeJsonFile);
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

        TestSettings::BracketingTranslator translator;
        QCoreApplication::installTranslator(&translator);
        mpPreferences->slot_guiLanguageChanged(MudletApp::getInterfaceLanguage());
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

        TestSettings::BracketingTranslator translator;
        QCoreApplication::installTranslator(&translator);
        mpPreferences->slot_guiLanguageChanged(MudletApp::getInterfaceLanguage());
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
        // Measured off the names, so only "wider than a rail" can be asked of it
        QVERIFY2(fullWidth > 100, qPrintable(qsl("the sidebar is only %1px wide at 1060x760, so this case cannot tell a collapse from where it started").arg(fullWidth)));

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
        QVERIFY2(pageOf(qsl("accessibility"))->widget()->minimumSizeHint().width() <= 640,
                 qPrintable(qsl("the German Accessibility page still needs %1px with the screen reader checkbox wrapped").arg(pageOf(qsl("accessibility"))->widget()->minimumSizeHint().width())));
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

    // Unnamed, a combo box is announced by its current item: what is picked, not what is being picked
    void test_theEditorThemePickersAreNamedForAScreenReader()
    {
        for (QComboBox* pPicker : {mpPreferences->code_editor_theme_selection_combobox, mpPreferences->script_preview_combobox}) {
            QVERIFY2(!pPicker->accessibleName().isEmpty(), qPrintable(qsl("%1 has nothing for a screen reader to announce it as").arg(pPicker->objectName())));
            QVERIFY2(!pPicker->accessibleDescription().isEmpty(), qPrintable(qsl("%1 has no accessible description").arg(pPicker->objectName())));
        }
        QVERIFY(mpPreferences->code_editor_theme_selection_combobox->accessibleName() != mpPreferences->script_preview_combobox->accessibleName());
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
    // width - see test_theNamesShowAtEveryWidthFromTheThresholdUp().
    void test_aPageWiderThanTheReadingColumnKeepsOneBeforeTheSidebarGoes()
    {
        delete mpPreferences;
        mpPreferences = nullptr;
        auto* pGerman = new QTranslator(qApp);
        QVERIFY2(pGerman->load(qsl("mudlet_de_DE"), qsl(":/lang")), "no German translation in the binary's resources, so no page is wider than the reading column to measure against");
        QVERIFY(qApp->installTranslator(pGerman));
        // ...at the 13pt macOS reads the interface at: at the offscreen platform's own size every German page fits
        const QFont priorFont = QApplication::font();
        QApplication::setFont(interfaceFontAt13pt());
        auto removeTranslator = qScopeGuard([pGerman, priorFont]() {
            QApplication::setFont(priorFont);
            qApp->removeTranslator(pGerman);
            delete pGerman;
        });
        openPreferences();

        auto* pSidebar = mpPreferences->findChild<QWidget*>(qsl("settingsSidebar"));
        QVERIFY2(pSidebar, "the settings shell has no sidebar to collapse");
        mpPreferences->resize(1500, 760);
        qApp->processEvents();
        const int fullSidebar = pSidebar->width();
        QVERIFY2(fullSidebar > 100, qPrintable(qsl("the sidebar is only %1px wide at 1500x760, so this case cannot tell a collapse from where it started").arg(fullSidebar)));

        // The pages this can be asked about at all: the ones a translation has
        // pushed past the reading column
        QStringList wide;
        for (const QString& key : specOrderedCategories()) {
            selectCategory(key);
            qApp->processEvents();
            if (pageOf(key)->widget()->minimumSizeHint().width() > 640) {
                wide.append(key);
            }
        }
        QVERIFY2(!wide.isEmpty(), "no German page needs more than the 640px reading column, so the breakpoint cannot be wrong in the way this case describes");

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

    void test_aWindowWiderThanTheShellNeedsWidensThePage()
    {
        selectCategory(qsl("general"));
        const int fullyExpanded = mpPreferences->sidebarWidths().fullyExpanded;
        QScrollArea* pPage = pageOf(qsl("general"));
        QWidget* pColumn = pPage->widget();
        const QList<QGroupBox*> cards = pColumn->findChildren<QGroupBox*>(QString(), Qt::FindDirectChildrenOnly);
        QVERIFY2(!cards.isEmpty(), "the General page has no cards to watch widen");

        mpPreferences->resize(fullyExpanded, 760);
        qApp->processEvents();
        QCOMPARE(mpPreferences->width(), fullyExpanded);
        const int columnBefore = pColumn->width();

        const int wider = fullyExpanded + 400;
        mpPreferences->resize(wider, 760);
        qApp->processEvents();
        QVERIFY2(mpPreferences->width() == wider, qPrintable(qsl("the window was offered %1px and took %2px").arg(wider).arg(mpPreferences->width())));
        auto* pSidebar = mpPreferences->findChild<QWidget*>(qsl("settingsSidebar"));
        QVERIFY2(pSidebar && !sidebar()->property(uiDesign::scmProp_rail).toBool(), "the names are not on show in a window wider than they and the widest page need");

        // A scrollbar the shorter page no longer needs hands its width back as well
        QVERIFY2(pColumn->width() >= columnBefore + 400, qPrintable(qsl("the window grew by 400px and the General page's column only from %1px to %2px").arg(columnBefore).arg(pColumn->width())));
        QCOMPARE(pColumn->width(), pPage->viewport()->width());
        for (auto* pCard : cards) {
            if (!pCard->isVisible()) {
                continue;
            }
            QVERIFY2(pCard->width() == pColumn->width(), qPrintable(qsl("the '%1' card is %2px wide in a %3px column").arg(pCard->objectName()).arg(pCard->width()).arg(pColumn->width())));
        }
        QVERIFY(!pPage->horizontalScrollBar()->isVisible());

        auto* pContent = mpPreferences->findChild<QWidget*>(qsl("settingsContent"));
        const int pageRight = pPage->mapTo(mpPreferences, QPoint(pPage->width(), 0)).x();
        const int margin = pContent->layout()->contentsMargins().right();
        QVERIFY2(mpPreferences->width() - pageRight <= margin,
                 qPrintable(qsl("%1px of the %2px window is empty to the right of the page, where only the %3px margin should be")
                                    .arg(mpPreferences->width() - pageRight)
                                    .arg(mpPreferences->width())
                                    .arg(margin)));
        mpPreferences->resize(1060, 760);
    }

    // Regression: narrowing the window from its widest folded the names at the first pixel
    void test_theNamesShowAtEveryWidthFromTheThresholdUp()
    {
        delete mpPreferences;
        mpPreferences = nullptr;
        // German at macOS's 13pt, where some pages are still wider than the reading column
        auto* pGerman = new QTranslator(qApp);
        QVERIFY2(pGerman->load(qsl("mudlet_de_DE"), qsl(":/lang")), "no German translation in the binary's resources, so no page is wider than the reading column");
        QVERIFY(qApp->installTranslator(pGerman));
        const QFont priorFont = QApplication::font();
        QApplication::setFont(interfaceFontAt13pt());
        auto removeTranslator = qScopeGuard([pGerman, priorFont]() {
            QApplication::setFont(priorFont);
            qApp->removeTranslator(pGerman);
            delete pGerman;
        });
        openPreferences();

        QString narrowPage;
        QString widePage;
        for (const QString& key : specOrderedCategories()) {
            const int minimum = pageOf(key)->widget()->minimumSizeHint().width();
            if (minimum <= 640 && narrowPage.isEmpty()) {
                narrowPage = key;
            }
            if (minimum > 640 && widePage.isEmpty()) {
                widePage = key;
            }
        }
        QVERIFY2(!narrowPage.isEmpty() && !widePage.isEmpty(),
                 qPrintable(qsl("this case needs a German page within the reading column and one wider than it, and found '%1' and '%2'").arg(narrowPage, widePage)));

        for (const QString& page : {narrowPage, widePage}) {
            selectCategory(page);
            qApp->processEvents();
            mpPreferences->resize(mpPreferences->sidebarWidths().fullyExpanded + 400, 760);
            qApp->processEvents();
            // Read once the page has been shown and laid out: its check boxes are wrapped again after its first show
            const auto widths = mpPreferences->sidebarWidths();
            for (int width = widths.fullyExpanded + 400; width >= widths.collapseBelow; width -= 16) {
                mpPreferences->resize(width, 760);
                qApp->processEvents();
                QVERIFY2(!sidebar()->property(uiDesign::scmProp_rail).toBool(),
                         qPrintable(qsl("the names folded in a %1px window on the '%2' page, which holds the %3px they need").arg(mpPreferences->width()).arg(page).arg(widths.collapseBelow)));
            }
            mpPreferences->resize(widths.collapseBelow, 760);
            qApp->processEvents();
            QVERIFY2(!sidebar()->property(uiDesign::scmProp_rail).toBool(), qPrintable(qsl("the names folded at the very width they need on the '%1' page").arg(page)));
            if (widths.collapseBelow - 1 >= mpPreferences->minimumWidth()) {
                mpPreferences->resize(widths.collapseBelow - 1, 760);
                qApp->processEvents();
                QVERIFY2(sidebar()->property(uiDesign::scmProp_rail).toBool(), qPrintable(qsl("the names stayed one pixel short of the width they need on the '%1' page").arg(page)));
            }
        }
        mpPreferences->resize(1060, 760);
    }

    // Reported against the redesign: picking a language collapsed the sidebar,
    // on a window nobody had touched. The window widens after the longer names,
    // which needs the wide offscreen screen CMakeLists.txt gives this class.
    void test_aLanguageChangeDoesNotCollapseTheSidebar()
    {
        auto* pSidebar = mpPreferences->findChild<QWidget*>(qsl("settingsSidebar"));
        QVERIFY2(pSidebar, "the settings shell has no sidebar to collapse");
        mpPreferences->resize(mpPreferences->sidebarWidths().fullyExpanded, 760);
        qApp->processEvents();
        const int widthBefore = mpPreferences->width();
        const int fullSidebar = pSidebar->width();
        QVERIFY2(fullSidebar > 100, qPrintable(qsl("the sidebar is only %1px wide before the language changes, so this case cannot tell a collapse from where it started").arg(fullSidebar)));

        auto* pGerman = new QTranslator(qApp);
        QVERIFY2(pGerman->load(qsl("mudlet_de_DE"), qsl(":/lang")), "no German translation in the binary's resources, so nothing here would lengthen a string");
        QVERIFY(qApp->installTranslator(pGerman));
        mpPreferences->slot_guiLanguageChanged(MudletApp::getInterfaceLanguage());
        qApp->processEvents();
        // Read before the translator goes, so a failure cannot leave it
        // installed for the cases that follow
        const int sidebarAfter = pSidebar->width();
        const bool railAfter = TestSettings::sidebar(mpPreferences)->property(uiDesign::scmProp_rail).toBool();
        const int widthAfter = mpPreferences->width();
        qApp->removeTranslator(pGerman);
        delete pGerman;

        QVERIFY2(widthAfter >= widthBefore, qPrintable(qsl("changing the language narrowed the window from %1px to %2px").arg(widthBefore).arg(widthAfter)));
        QVERIFY2(!railAfter && sidebarAfter >= fullSidebar,
                 qPrintable(qsl("changing the language collapsed the sidebar from %1px to %2px, on a window %3px wide").arg(fullSidebar).arg(sidebarAfter).arg(widthAfter)));
    }

    // Regression: the Editor page's theme preview list grew to the profile's longest
    // item name and scrolled the page sideways before the names folded
    void test_theSidebarFoldsBeforeAnyPageScrollsSideways()
    {
        QVERIFY(mpHost->mLuaInterpreter.compileAndExecuteScript(qsl(
                "permScript(\"A script with a name as long as a real profile can give one, far too long for the theme preview list to be as wide as it\", \"\", \"-- shown in the theme preview\")")));
        // The preview list is filled as the dialog is built
        delete mpPreferences;
        mpPreferences = nullptr;
        openPreferences();
        walkTheWindowDown();
        if (QTest::currentTestFailed()) {
            return;
        }
        QWidget* pEditorColumn = pageOf(qsl("editor"))->widget();
        QVERIFY2(pEditorColumn->minimumSizeHint().width() <= 640,
                 qPrintable(qsl("one long item name in the profile widened the Editor page to %1px, past the 640px reading column").arg(pEditorColumn->minimumSizeHint().width())));
    }

    // German pushes pages past the reading column, where the names give way for the page
    void test_theSidebarFoldsBeforeAnyGermanPageScrollsSideways()
    {
        delete mpPreferences;
        mpPreferences = nullptr;
        auto* pGerman = new QTranslator(qApp);
        QVERIFY2(pGerman->load(qsl("mudlet_de_DE"), qsl(":/lang")), "no German translation in the binary's resources, so no page is wider than the reading column to walk past");
        QVERIFY(qApp->installTranslator(pGerman));
        auto removeTranslator = qScopeGuard([pGerman]() {
            qApp->removeTranslator(pGerman);
            delete pGerman;
        });
        openPreferences();
        walkTheWindowDown();
    }
};

#include "SettingsShellNavigationTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsShellNavigationTest)
