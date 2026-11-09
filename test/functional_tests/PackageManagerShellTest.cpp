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
 * Guards the package manager's design-language shell: sheets on the columns,
 * view chips kept in step with the hidden .ui buttons every slot presses, the
 * row delegate's accent bar and version, and the action buttons per view.
 *
 * Run with: ctest -R PackageManagerShellTest -V
 */

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QTabBar>
#include <QtTest/QtTest>


#include "GripSplitter.h"
#include "Host.h"
#include "MudletApp.h"
#include "PackageItemDelegate.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "dlgPackageManager.h"
#include "dlgSystemMessageArea.h"
#include "mudlet.h"
#include "uiDesign.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class PackageManagerShellTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    Host* mpHost = nullptr;
    dlgPackageManager* mpDialog = nullptr;
    const QString mProfileName = qsl("PackageManagerShell-Test-Profile");

    // Seeded so the cases do not change with what a fresh profile preinstalls
    const QString mFirstPackage = qsl("aaa-shell-test-package");
    const QString mFirstTitle = qsl("The first package this test lists");
    const QString mSecondPackage = qsl("zzz-shell-test-package");

    // A styled fill is composited, and the 3px bar has an antialiased corner at
    // each end, so the other style tests' hairline slack applies
    static constexpr int scmFillTolerance = 24;

    // With a version it also lists the first package at that version: Updates
    // only counts a package installed *and* listed higher on the repository
    bool writeRepositoryIndex(const QString& firstPackageVersion = QString()) const
    {
        QJsonArray packages;
        for (const QString& name : {qsl("explore-only-one"), qsl("explore-only-two")}) {
            QJsonObject entry;
            entry.insert(qsl("mpackage"), name);
            entry.insert(qsl("filename"), qsl("%1.mpackage").arg(name));
            entry.insert(qsl("version"), qsl("1.0.0"));
            entry.insert(qsl("title"), qsl("a package that exists only in this test"));
            entry.insert(qsl("author"), qsl("PackageManagerShellTest"));
            packages.append(entry);
        }
        if (!firstPackageVersion.isEmpty()) {
            QJsonObject entry;
            entry.insert(qsl("mpackage"), mFirstPackage);
            entry.insert(qsl("filename"), qsl("%1.mpackage").arg(mFirstPackage));
            entry.insert(qsl("version"), firstPackageVersion);
            entry.insert(qsl("title"), mFirstTitle);
            entry.insert(qsl("author"), qsl("A Shell Test"));
            packages.append(entry);
        }
        QJsonObject root;
        root.insert(qsl("packages"), packages);

        QFile file(qsl("%1/mpkg.packages.json").arg(MudletApp::getMudletPath(enums::profileHomePath, mProfileName)));
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        return file.write(QJsonDocument(root).toJson()) > 0;
    }

    QTabBar* viewBar() const { return mpDialog->findChild<QTabBar*>(qsl("packagesViewBar")); }

    int checkedView() const
    {
        if (mpDialog->pushButton_explore->isChecked()) {
            return static_cast<int>(NavigationView::Explore);
        }
        if (mpDialog->pushButton_installed->isChecked()) {
            return static_cast<int>(NavigationView::Installed);
        }
        if (mpDialog->pushButton_updates->isChecked()) {
            return static_cast<int>(NavigationView::Updates);
        }
        return -1;
    }

    void showView(const NavigationView view)
    {
        QAbstractButton* pButton = view == NavigationView::Explore ? static_cast<QAbstractButton*>(mpDialog->pushButton_explore)
                                                                   : (view == NavigationView::Installed ? static_cast<QAbstractButton*>(mpDialog->pushButton_installed)
                                                                                                        : static_cast<QAbstractButton*>(mpDialog->pushButton_updates));
        pButton->click();
        TestPixels::settle();
    }

    QRect rowRect(const int row) const { return mpDialog->packageList->visualItemRect(mpDialog->packageList->item(row)); }

    // A new profile preinstalls Mudlet's own packages and the list is sorted,
    // so a package's row cannot be counted out
    int rowOf(const QString& packageName) const
    {
        for (int row = 0; row < mpDialog->packageList->count(); ++row) {
            if (mpDialog->packageList->item(row)->text() == packageName) {
                return row;
            }
        }
        return -1;
    }

    static constexpr int scmRowVersionZone = 56;

    QPair<QColor, int> versionPixel(const QImage& shot, const int row, const QColor& ink) const
    {
        const QRect rect = rowRect(row);
        QColor nearest;
        int closest = -1;
        for (int y = rect.top() + 2; y < rect.center().y(); ++y) {
            for (int x = rect.right() - scmRowVersionZone; x < rect.right() - 4; ++x) {
                const QColor read = TestPixels::sampled(shot, QPoint(x, y));
                if (const int apart = TestPixels::distance(read, ink); closest < 0 || apart < closest) {
                    closest = apart;
                    nearest = read;
                }
            }
        }
        return {nearest, closest};
    }

    PackageItemDelegate* rowDelegate() const { return qobject_cast<PackageItemDelegate*>(mpDialog->packageList->itemDelegate()); }

    // Given the option a view hands the delegate, so the name is cut to the same room
    QString nameDrawn(const int row) const
    {
        QStyleOptionViewItem option;
        option.initFrom(mpDialog->packageList);
        option.rect = rowRect(row);
        return rowDelegate()->nameDrawn(option, mpDialog->packageList->model()->index(row, 0));
    }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.startMudlet(), qPrintable(mSession.why()));
        // Some assertions match English text
        mudlet::self()->setInterfaceLanguage(qsl("en_US"));

        QVERIFY2(mSession.openProfile(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();

        // One row with a summary and one without, since the two are drawn
        // differently; written straight onto the host since nothing installs here
        mpHost->mInstalledPackages << mFirstPackage << mSecondPackage;
        mpHost->mPackageInfo.insert(mFirstPackage,
                                    QMap<QString, QString>{{qsl("mpackage"), mFirstPackage},
                                                           {qsl("title"), mFirstTitle},
                                                           {qsl("author"), qsl("A Shell Test")},
                                                           {qsl("version"), qsl("1.2.3")},
                                                           {qsl("description"), qsl("# Heading\n\nA package with `code` in its notes.\n")}});
        mpHost->mPackageInfo.insert(mSecondPackage, QMap<QString, QString>{{qsl("mpackage"), mSecondPackage}, {qsl("version"), qsl("0.1")}});

        // Explore gets rows without the network; neither name is installed, so
        // the Installed view and the Updates count are untouched
        QVERIFY(writeRepositoryIndex());

        mpDialog = new dlgPackageManager(nullptr, mpHost);
        QVERIFY2(mpDialog, "the package manager was not built at all");
        mpDialog->resize(900, 600);
        mpDialog->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpDialog));
        TestPixels::settle();

        QVERIFY2(mpDialog->packageList->count() >= 2,
                 qPrintable(qsl("SETUP: the Installed view lists %1 packages, so the rows below have nothing to be read on").arg(QString::number(mpDialog->packageList->count()))));
    }

    void cleanupTestCase()
    {
        if (mpDialog) {
            mpDialog->close();
            mpDialog = nullptr;
        }
        QCoreApplication::processEvents();
        mpHost = nullptr;
        if (mudlet::self()) {
            mudlet::self()->setAppearance(enums::Appearance::systemSetting);
        }
        mSession.stop();
    }

    // QTabBar makes its first tab current and emits currentChanged, which
    // pressed Explore before the window had chosen a view
    void test_theWindowOpensWithTheInstalledChipLit()
    {
        dlgPackageManager fresh(nullptr, mpHost);
        auto* pBar = fresh.findChild<QTabBar*>(qsl("packagesViewBar"));
        QVERIFY2(pBar, "the fresh window has no chip row");
        qInfo().noquote() << qsl("  a fresh window's chip row reads tab %1, the hidden Installed button is %2checked")
                                     .arg(pBar->currentIndex())
                                     .arg(fresh.pushButton_installed->isChecked() ? QString() : qsl("not "));
        QVERIFY2(pBar->currentIndex() == static_cast<int>(NavigationView::Installed),
                 qPrintable(qsl("a fresh window lights chip %1 where the Installed view (%2) is what it opens on").arg(pBar->currentIndex()).arg(static_cast<int>(NavigationView::Installed))));
        QVERIFY2(fresh.pushButton_installed->isChecked(), "a fresh window's hidden Installed button is not the checked one");
        QVERIFY2(fresh.mCurrentView == NavigationView::Installed, "a fresh window's view is not Installed");
    }

    // Includes a seam moved by setSizes(), which emits no splitterMoved
    void test_theHeadlineFitsTheRoomTheSeamLeavesIt()
    {
        showView(NavigationView::Installed);
        QVERIFY2(mpDialog->mpSplitter, "there is no splitter to move");
        const QList<int> before = mpDialog->mpSplitter->sizes();
        // Long enough to be cut beside a narrow column and whole beside a wide one
        const QString longName = qsl("a-package-name-long-enough-to-be-cut");
        mpDialog->fillPackageDetails(longName, qsl("A summary"), qsl("An author"), qsl("1.0"));
        QCoreApplication::processEvents();

        mpDialog->mpSplitter->setSizes({mpDialog->width() - 330, 330});
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        const QString cramped = mpDialog->label_packageName->text();

        mpDialog->mpSplitter->setSizes({300, mpDialog->width() - 300});
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        const QString roomy = mpDialog->label_packageName->text();
        qInfo().noquote() << qsl("  the headline reads \"%1\" with the details column narrow and \"%2\" with it wide").arg(cramped, roomy);

        QVERIFY2(cramped != longName, qPrintable(qsl("beside a 330px column the headline still reads the whole \"%1\", so the narrow case reads nothing").arg(cramped)));
        QVERIFY2(roomy == longName,
                 qPrintable(qsl("with the details column wide again the headline still reads \"%1\" rather than the whole name \"%2\" - the name was cut for a room the column no longer has")
                                    .arg(roomy, longName)));
        mpDialog->mpSplitter->setSizes(before);
        QCoreApplication::processEvents();
    }

    // An install can show a package's toolbars, dock widgets of the main window
    // that bring it forward over this dialog
    void test_theWindowComesBackToTheFrontAfterItsOwnActs()
    {
        mudlet::self()->show();
        mudlet::self()->raise();
        mudlet::self()->activateWindow();
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        if (QApplication::activeWindow() != mudlet::self()) {
            QSKIP("this platform does not report window activation, so nothing here can be read");
        }

        mpDialog->comeBackToFront();
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        QVERIFY2(QApplication::activeWindow() == mpDialog,
                 qPrintable(qsl("after asking for the front the active window is %1 rather than the package manager")
                                    .arg(QApplication::activeWindow() ? QApplication::activeWindow()->objectName() : qsl("none"))));
    }

    void test_websiteOpensThePackagesOwnPage()
    {
        QCOMPARE(mpDialog->packageWebsiteUrl(qsl("achaea-room-tracking")), qsl("https://packages.mudlet.org/packages/achaea-room-tracking"));
        QCOMPARE(mpDialog->packageWebsiteUrl(qsl("Achaean System")), qsl("https://packages.mudlet.org/packages/achaean-system"));
        QCOMPARE(mpDialog->packageWebsiteUrl(qsl("generic_mapper")), qsl("https://packages.mudlet.org/packages/generic-mapper"));
        QCOMPARE(mpDialog->packageWebsiteUrl(qsl("AchaeaChatTabs")), qsl("https://packages.mudlet.org/packages/achaeachattabs"));
        QCOMPARE(mpDialog->packageWebsiteUrl(qsl("  odd -- name! ")), qsl("https://packages.mudlet.org/packages/odd-name"));

        // Its own help URL is taken only for the repository's package, the same name by the same author
        QVERIFY2(writeRepositoryIndex(qsl("1.2.3")), "SETUP: the repository index naming the installed package could not be written");
        QVERIFY2(mpDialog->readPackageRepositoryFile(), "SETUP: the repository index naming the installed package could not be read back");
        QMap<QString, QString> withHelp = mpHost->mPackageInfo.value(mFirstPackage);
        withHelp.insert(qsl("helpURL"), qsl("https://example.org/help"));
        mpHost->mPackageInfo.insert(mFirstPackage, withHelp);
        QCOMPARE(mpDialog->packageWebsiteUrl(mFirstPackage), qsl("https://example.org/help"));
        withHelp.remove(qsl("helpURL"));
        mpHost->mPackageInfo.insert(mFirstPackage, withHelp);
        QVERIFY(writeRepositoryIndex());
        QVERIFY(mpDialog->readPackageRepositoryFile());
    }

    void test_theViewIsARowOfChipsThatStaysInStepWithTheButtons()
    {
        QTabBar* pBar = viewBar();
        QVERIFY2(pBar, "there is no QTabBar named 'packagesViewBar' - the view is not drawn as a row of chips at all");
        QCOMPARE(pBar->count(), 3);
        QVERIFY2(!mpDialog->headerBar->isVisible(), "the old row of navigation buttons is still on show beside the chips");
        QVERIFY2(!pBar->accessibleName().isEmpty(), "the strip of chips has no accessible name, so a screen reader announces an unnamed tab bar");

        showView(NavigationView::Explore);
        qInfo().noquote() << qsl("  the Explore button was pressed; the bar reads tab %1").arg(QString::number(pBar->currentIndex()));
        QVERIFY2(pBar->currentIndex() == static_cast<int>(NavigationView::Explore),
                 qPrintable(qsl("pressing the hidden Explore button left the chip row on tab %1 rather than on %2")
                                    .arg(QString::number(pBar->currentIndex()), QString::number(static_cast<int>(NavigationView::Explore)))));

        pBar->setCurrentIndex(static_cast<int>(NavigationView::Installed));
        TestPixels::settle();
        qInfo().noquote() << qsl("  the Installed chip was chosen; the buttons read view %1").arg(QString::number(checkedView()));
        QVERIFY2(checkedView() == static_cast<int>(NavigationView::Installed),
                 qPrintable(qsl("choosing the Installed chip left the hidden buttons on view %1 rather than on %2")
                                    .arg(QString::number(checkedView()), QString::number(static_cast<int>(NavigationView::Installed)))));
        QVERIFY2(mpDialog->packageList->count() >= 2, "choosing the Installed chip did not bring the installed packages back into the list");
    }

    void test_theUpdatesChipCarriesWhateverTheButtonSays()
    {
        QTabBar* pBar = viewBar();
        QVERIFY(pBar);
        const int updates = static_cast<int>(NavigationView::Updates);
        qInfo().noquote() << qsl("  the Updates button says \"%1\", the chip says \"%2\"").arg(mpDialog->pushButton_updates->text(), pBar->tabText(updates));
        QVERIFY2(pBar->tabText(updates) == mpDialog->pushButton_updates->text(),
                 qPrintable(qsl("the Updates chip says \"%1\" where the button it stands for says \"%2\"").arg(pBar->tabText(updates), mpDialog->pushButton_updates->text())));
    }

    void test_theSheetsAreSetOnTheColumnsRatherThanOnTheWindow()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();

        QVERIFY2(mpDialog->styleSheet().isEmpty(), qPrintable(qsl("the dialog itself carries a sheet, which a profile's Lua stylesheet replaces on show: %1").arg(mpDialog->styleSheet().left(120))));

        const QString left = mpDialog->leftPanel->styleSheet();
        QVERIFY2(left.contains(tokens.pane.name()), qPrintable(qsl("the list column's sheet never names the pane tone %1, so it is not told apart from the page beside it").arg(tokens.pane.name())));
        QVERIFY2(left.contains(qsl("QTabBar::tab")), qPrintable(qsl("the list column's sheet draws no chips for the view strip: %1").arg(left.left(200))));
        QVERIFY2(left.contains(qsl("QLineEdit")), qPrintable(qsl("the list column's sheet draws no field, so the search box is platform-drawn: %1").arg(left.left(200))));
        QVERIFY2(left.contains(qsl("QPushButton")), "the list column's sheet draws no button, so Install from a file is platform-drawn");

        const QString rows = mpDialog->packageList->styleSheet();
        const QString chosenWash = uiDesign::blend(tokens.pane, tokens.accent, uiDesign::scmAccentWashStrength).name();
        qInfo().noquote() << qsl("  the chosen row is washed with %1").arg(chosenWash);
        QVERIFY2(rows.contains(chosenWash), qPrintable(qsl("the list's sheet never names %1, the wash the design fills a chosen row with: %2").arg(chosenWash, rows.left(240))));
        QVERIFY2(rows.contains(qsl("QScrollBar")), "the list's own scrollbars are left platform-drawn");

        const QString right = mpDialog->rightPanel->styleSheet();
        QVERIFY2(right.contains(tokens.page.name()), qPrintable(qsl("the details column's sheet never names the page tone %1").arg(tokens.page.name())));
        QVERIFY2(right.contains(qsl("QPushButton")), qPrintable(qsl("the details column's sheet draws no button: %1").arg(right.left(200))));
        QVERIFY2(right.contains(qsl("QTextEdit")), qPrintable(qsl("the details column's sheet draws no field, so the notes are platform-drawn: %1").arg(right.left(200))));
    }

    void test_theActionButtonsFollowTheView()
    {
        showView(NavigationView::Installed);
        QVERIFY2(!mpDialog->pushButton_installRepo->isVisible(), "Install is on show in the Installed view, where there is nothing for it to install");
        QVERIFY2(mpDialog->pushButton_remove->isVisible(), "Remove is not on show in the Installed view, which is the one view it applies to");

        showView(NavigationView::Explore);
        QVERIFY2(mpDialog->pushButton_installRepo->isVisible(), "Install is not on show in the Explore view");
        QVERIFY2(!mpDialog->pushButton_remove->isVisible(), "Remove is on show in the Explore view, where nothing listed is installed");

        showView(NavigationView::Installed);
        mpDialog->packageList->setCurrentRow(0);
        TestPixels::settle();
        for (const QPushButton* pButton : {mpDialog->pushButton_website, mpDialog->pushButton_report}) {
            QVERIFY2(!pButton->text().isEmpty(), qPrintable(qsl("%1 is a picture with no word beside it").arg(pButton->objectName())));
            QVERIFY2(!pButton->icon().isNull(), qPrintable(qsl("%1 carries no glyph").arg(pButton->objectName())));
            QVERIFY2(!pButton->isFlat(), qPrintable(qsl("%1 is still flat, so it reads as decoration rather than as somewhere to press").arg(pButton->objectName())));
            QVERIFY2(!pButton->toolTip().isEmpty(), qPrintable(qsl("%1 says nothing about where it leads").arg(pButton->objectName())));
        }
        QVERIFY2(!mpDialog->pushButton_installFile->icon().isNull(), "Install from a file carries no glyph");
        QCOMPARE(mpDialog->pushButton_installFile->text(), qsl("Install from a file"));
    }

    void test_theRowsAreTheDesignsRows()
    {
        showView(NavigationView::Installed);
        mpDialog->packageList->setCurrentRow(0);
        TestPixels::settle();

        QListWidgetItem* pRow = mpDialog->packageList->item(0);
        QVERIFY2(pRow, "the Installed view lists nothing at all");
        QCOMPARE(pRow->text(), mFirstPackage);
        QVERIFY2(pRow->data(Qt::UserRole).toString() == mFirstTitle,
                 qPrintable(qsl("the row's second line reads \"%1\" rather than the package's title \"%2\"").arg(pRow->data(Qt::UserRole).toString(), mFirstTitle)));

        QStyleOptionViewItem option;
        option.initFrom(mpDialog->packageList);
        option.rect = mpDialog->packageList->visualItemRect(pRow);
        const int height = mpDialog->packageList->itemDelegate()->sizeHint(option, mpDialog->packageList->model()->index(0, 0)).height();
        QFont caption = option.font;
        caption.setPointSize(uiDesign::typeSize(uiDesign::TypeStep::Caption));
        const int twoLines = QFontMetrics(option.font).height() + QFontMetrics(caption).height();
        qInfo().noquote() << qsl("  a row measures %1px against %2px of words on two lines").arg(QString::number(height), QString::number(twoLines));
        QVERIFY2(height >= twoLines,
                 qPrintable(qsl("a row is %1px tall where its name and its summary alone come to %2px - the summary has nowhere to be drawn").arg(QString::number(height), QString::number(twoLines))));

        const QRect row = mpDialog->packageList->visualItemRect(pRow);
        QVERIFY2(!row.isEmpty(), "the chosen row has no rectangle on the viewport, so there is nothing to read");
        const QImage shot = mpDialog->packageList->viewport()->grab().toImage();
        const QColor bar = shot.pixelColor(row.left() + 1, row.center().y());
        const QColor accent = uiDesign::themeTokens().accent;
        qInfo().noquote() << qsl("  the chosen row's gutter reads %1 against an accent of %2").arg(bar.name(), accent.name());
        QVERIFY2(TestPixels::readsAs(bar, accent, scmFillTolerance),
                 qPrintable(qsl("the chosen row's leading gutter reads %1 where the accent is %2 - the bar that says which row is chosen is not being painted").arg(bar.name(), accent.name())));
    }

    void test_theNoticeIsTheSharedOneAndIsQuietAtRest()
    {
        auto* pNotice = mpDialog->findChild<dlgSystemMessageArea*>(qsl("packagesNotice"));
        QVERIFY2(pNotice, "there is no dlgSystemMessageArea named 'packagesNotice' - the window reports failures with a bare label");
        QVERIFY2(mpDialog->leftPanel->isAncestorOf(pNotice), "the notice is not in the list column, where what it reports happened");
        QVERIFY2(!pNotice->isVisible(), "the notice is on show with nothing to report");
        QVERIFY2(!mpDialog->label_importStatus->isVisible(), "the label the notice replaced is still on show");
    }

    void test_theHeadOfTheDetailsColumnIsTheTypeScale()
    {
        showView(NavigationView::Installed);
        mpDialog->packageList->setCurrentRow(0);
        TestPixels::settle();

        const QFont nameFont = mpDialog->label_packageName->font();
        qInfo().noquote() << qsl("  the package name is set at %1pt weight %2, against a Title step of %3pt")
                                     .arg(QString::number(nameFont.pointSize()), QString::number(nameFont.weight()), QString::number(uiDesign::typeSize(uiDesign::TypeStep::Title)));
        QVERIFY2(nameFont.pointSize() == uiDesign::typeSize(uiDesign::TypeStep::Title),
                 qPrintable(qsl("the package name is set at %1pt where the type scale's Title step is %2pt")
                                    .arg(QString::number(nameFont.pointSize()), QString::number(uiDesign::typeSize(uiDesign::TypeStep::Title)))));
        QVERIFY2(nameFont.weight() >= QFont::DemiBold,
                 qPrintable(qsl("the package name is set at weight %1, which is lighter than the DemiBold the design sets a title in").arg(QString::number(nameFont.weight()))));

        QCOMPARE(mpDialog->label_icon->size(), QSize(48, 48));
        QVERIFY2(!mpDialog->line->isVisible(), "the rule across the details column is still drawn, beside the notes field's own hairline");
        QVERIFY2(mpDialog->label_author->text().contains(qsl("A Shell Test")), qPrintable(qsl("the caption line does not name the author: \"%1\"").arg(mpDialog->label_author->text())));
        QVERIFY2(mpDialog->label_version->text().contains(qsl("1.2.3")), qPrintable(qsl("the caption line does not name the version: \"%1\"").arg(mpDialog->label_version->text())));
    }

    // Built through buildPackageContextMenu() rather than shown: QMenu::exec()
    // runs its own event loop, which a test cannot come back out of
    void test_theListsRightClickMenuCarriesTheViewsActs()
    {
        showView(NavigationView::Installed);
        const int row = rowOf(mFirstPackage);
        QVERIFY2(row >= 0, "SETUP: the package this case reads is not listed in the Installed view");
        mpDialog->packageList->setCurrentRow(row);
        TestPixels::settle();

        QMenu* pMenu = mpDialog->buildPackageContextMenu(mpDialog->packageList->item(row));
        QVERIFY2(pMenu, "no menu is built at all over an installed package's row");
        QStringList entries;
        for (const QAction* pAction : pMenu->actions()) {
            entries << qsl("%1 (%2)").arg(pAction->text(), pAction->objectName());
        }
        qInfo().noquote() << qsl("  over an installed package the menu carries: %1").arg(entries.join(qsl(", ")));

        QVERIFY2(pMenu->findChild<QAction*>(qsl("packagesMenuRemove")), "the menu over an installed package carries no entry named 'packagesMenuRemove'");
        delete pMenu;

        showView(NavigationView::Explore);
        mpDialog->packageList->setCurrentRow(0);
        TestPixels::settle();
        QVERIFY2(mpDialog->packageList->count() >= 1, "SETUP: the Explore view lists nothing, so there is no row to open a menu over");
        QMenu* pExploreMenu = mpDialog->buildPackageContextMenu(mpDialog->packageList->item(0));
        QVERIFY2(pExploreMenu, "no menu is built over an Explore row");
        QVERIFY2(pExploreMenu->findChild<QAction*>(qsl("packagesMenuInstall")), "the menu over an Explore row carries no entry named 'packagesMenuInstall' - the one act that view offers");
        QVERIFY2(!pExploreMenu->findChild<QAction*>(qsl("packagesMenuRemove")), "the menu over an Explore row offers Remove, where nothing listed is installed to be removed");
        delete pExploreMenu;

        showView(NavigationView::Installed);
        mpDialog->packageList->setCurrentRow(0);
        TestPixels::settle();
    }

    void test_theVersionStandsBesideTheNameInTheOkTone()
    {
        showView(NavigationView::Installed);
        const int row = rowOf(mFirstPackage);
        QVERIFY2(row >= 0, "SETUP: the package this case reads is not listed in the Installed view");
        mpDialog->packageList->setCurrentRow(row);
        TestPixels::settle();

        QLabel* pName = mpDialog->label_packageName;
        QLabel* pVersion = mpDialog->label_version;
        QVERIFY2(pName->parentWidget() == pVersion->parentWidget(),
                 qPrintable(qsl("the version sits on %1 where the name sits on %2 - they are not on one row")
                                    .arg(pVersion->parentWidget() ? pVersion->parentWidget()->objectName() : qsl("nothing"),
                                         pName->parentWidget() ? pName->parentWidget()->objectName() : qsl("nothing"))));
        qInfo().noquote() << qsl("  the name is at %1,%2 %3x%4 and the version at %5,%6 %7x%8")
                                     .arg(QString::number(pName->x()),
                                          QString::number(pName->y()),
                                          QString::number(pName->width()),
                                          QString::number(pName->height()),
                                          QString::number(pVersion->x()),
                                          QString::number(pVersion->y()),
                                          QString::number(pVersion->width()),
                                          QString::number(pVersion->height()));
        QVERIFY2(std::abs(pName->y() - pVersion->y()) <= 1,
                 qPrintable(qsl("the name starts at y %1 and the version at y %2 - the version is on a line of its own rather than beside the name")
                                    .arg(QString::number(pName->y()), QString::number(pVersion->y()))));
        QVERIFY2(pVersion->x() >= pName->geometry().right(),
                 qPrintable(qsl("the version starts at x %1 where the name ends at x %2 - it does not stand after the name on the row")
                                    .arg(QString::number(pVersion->x()), QString::number(pName->geometry().right()))));

        QVERIFY2(pVersion->text() == qsl("1.2.3"), qPrintable(qsl("the version beside the name reads \"%1\" rather than the bare number").arg(pVersion->text())));
        QVERIFY2(pVersion->font().pointSize() == mpDialog->font().pointSize(),
                 qPrintable(qsl("the version is set at %1pt where the window's body font is %2pt").arg(QString::number(pVersion->font().pointSize()), QString::number(mpDialog->font().pointSize()))));

        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QColor walked = uiDesign::readableOn(tokens.page, uiDesign::stateColor(uiDesign::scmStateHue_ok, tokens.darkPage), tokens.text, uiDesign::scmTextMinimumRatio);
        const QString right = mpDialog->rightPanel->styleSheet();
        qInfo().noquote() << qsl("  the version's ink walks the ok tone %1 to %2 against the page %3")
                                     .arg(uiDesign::stateColor(uiDesign::scmStateHue_ok, tokens.darkPage).name(), walked.name(), tokens.page.name());
        QVERIFY2(right.contains(qsl("#label_version { color: %1").arg(walked.name())),
                 qPrintable(qsl("the details column's sheet does not write the version in %1, the design's ok tone walked to the text floor: %2").arg(walked.name(), right.left(400))));

        // The fixture's index lists nothing installed, so one listing an update
        // is written for this case and taken away again
        QVERIFY2(writeRepositoryIndex(qsl("9.9.9")), "SETUP: the repository index listing an update could not be written");
        QVERIFY2(mpDialog->readPackageRepositoryFile(), "SETUP: the repository index listing an update could not be read back");
        showView(NavigationView::Updates);
        const int updateRow = rowOf(mFirstPackage);
        QVERIFY2(updateRow >= 0, "SETUP: the package listed at a higher version did not reach the Updates view");
        mpDialog->packageList->setCurrentRow(updateRow);
        TestPixels::settle();
        qInfo().noquote() << qsl("  in the Updates view the version reads \"%1\"").arg(pVersion->text());
        QVERIFY2(pVersion->text() == qsl("1.2.3 → 9.9.9"),
                 qPrintable(qsl("in the Updates view the version reads \"%1\" rather than the installed number, the arrow and the waiting one").arg(pVersion->text())));

        QVERIFY(writeRepositoryIndex());
        QVERIFY(mpDialog->readPackageRepositoryFile());
        showView(NavigationView::Installed);
        mpDialog->packageList->setCurrentRow(0);
        TestPixels::settle();
    }

    void test_everyRowCarriesItsVersionAtTheTrailingEdge()
    {
        showView(NavigationView::Installed);
        const int row = rowOf(mFirstPackage);
        QVERIFY2(row >= 0, "SETUP: the package this case reads is not listed in the Installed view");
        mpDialog->packageList->setCurrentRow(row);
        TestPixels::settle();

        QListWidgetItem* pRow = mpDialog->packageList->item(row);
        QVERIFY2(pRow->data(PackageItemDelegate::cPackageVersionRole).toString() == qsl("1.2.3"),
                 qPrintable(qsl("the row carries \"%1\" in its version role where the package is installed at 1.2.3 - nothing on the row says which version it is")
                                    .arg(pRow->data(PackageItemDelegate::cPackageVersionRole).toString())));

        QVERIFY2(rowDelegate(), "the list's delegate is not a PackageItemDelegate, so there is no row-version ink to read against");
        const QColor ink = rowDelegate()->versionInk(true);
        const QImage shot = TestPixels::shot(mpDialog->packageList->viewport());
        const auto [read, apart] = versionPixel(shot, row, ink);
        qInfo().noquote() << qsl("  the chosen row's trailing run reads nearest %1 against the version ink %2, %3 apart").arg(read.name(), ink.name(), QString::number(apart));
        QVERIFY2(TestPixels::readsAs(read, ink, scmFillTolerance),
                 qPrintable(qsl("nothing in the last %1px of the chosen row's name line reads as %2, the ink the version is written in - the nearest pixel is %3 - so the row draws no version")
                                    .arg(QString::number(scmRowVersionZone), ink.name(), read.name())));

        // Asked of the delegate rather than read off a grab: in pixels an
        // ellipsis cannot be told from a full stop
        qInfo().noquote() << qsl("  the row's name line reads \"%1\"").arg(nameDrawn(row));
        QVERIFY2(nameDrawn(row) == mFirstPackage,
                 qPrintable(qsl("the row's name line reads \"%1\" where the package is called \"%2\" - the version took room the name needed").arg(nameDrawn(row), mFirstPackage)));

        const int otherRow = rowOf(mSecondPackage);
        QVERIFY2(otherRow >= 0 && otherRow != row, "SETUP: the second package is not on a row of its own in the Installed view");
        QVERIFY2(mpDialog->packageList->item(otherRow)->data(PackageItemDelegate::cPackageVersionRole).toString() == qsl("0.1"),
                 qPrintable(qsl("the second package's row carries \"%1\" rather than its version 0.1")
                                    .arg(mpDialog->packageList->item(otherRow)->data(PackageItemDelegate::cPackageVersionRole).toString())));
        const QColor plainInk = rowDelegate()->versionInk(false);
        const auto [otherRead, otherApart] = versionPixel(shot, otherRow, plainInk);
        qInfo().noquote() << qsl("  the unchosen row's trailing run reads nearest %1 against %2, %3 apart").arg(otherRead.name(), plainInk.name(), QString::number(otherApart));
        QVERIFY2(TestPixels::readsAs(otherRead, plainInk, scmFillTolerance),
                 qPrintable(qsl("the unchosen row's version reads nearest %1 where the row's version ink is %2 - the number is not drawn in the ink a plain row is given")
                                    .arg(otherRead.name(), plainInk.name())));

        QVERIFY2(writeRepositoryIndex(), "SETUP: the repository index could not be written");
        showView(NavigationView::Explore);
        const int exploreRow = rowOf(qsl("explore-only-one"));
        QVERIFY2(exploreRow >= 0, "SETUP: the Explore view does not list the repository's own packages");
        QVERIFY2(mpDialog->packageList->item(exploreRow)->data(PackageItemDelegate::cPackageVersionRole).toString() == qsl("1.0.0"),
                 qPrintable(qsl("an Explore row carries \"%1\" rather than the version the repository lists, 1.0.0")
                                    .arg(mpDialog->packageList->item(exploreRow)->data(PackageItemDelegate::cPackageVersionRole).toString())));

        QVERIFY2(writeRepositoryIndex(qsl("9.9.9")), "SETUP: the repository index listing an update could not be written");
        QVERIFY2(mpDialog->readPackageRepositoryFile(), "SETUP: the repository index listing an update could not be read back");
        showView(NavigationView::Updates);
        const int updateRow = rowOf(mFirstPackage);
        QVERIFY2(updateRow >= 0, "SETUP: the package listed at a higher version did not reach the Updates view");
        QVERIFY2(mpDialog->packageList->item(updateRow)->data(PackageItemDelegate::cPackageVersionRole).toString() == qsl("1.2.3 → 9.9.9"),
                 qPrintable(qsl("an Updates row carries \"%1\" rather than the installed number, the arrow and the waiting one")
                                    .arg(mpDialog->packageList->item(updateRow)->data(PackageItemDelegate::cPackageVersionRole).toString())));

        QVERIFY(writeRepositoryIndex());
        QVERIFY(mpDialog->readPackageRepositoryFile());
        showView(NavigationView::Installed);
        mpDialog->packageList->setCurrentRow(0);
        TestPixels::settle();
    }

    void test_theTwoColumnsAreSplitByAHandleThatIsRemembered()
    {
        auto* pSplitter = mpDialog->findChild<uiDesign::GripSplitter*>(qsl("packagesSplitter"));
        QVERIFY2(pSplitter, "there is no uiDesign::GripSplitter named 'packagesSplitter' - the two columns are still a row that cannot be resized");
        QCOMPARE(pSplitter->count(), 2);
        QVERIFY2(pSplitter->widget(0) == mpDialog->leftPanel, "the list column is not the splitter's first pane");
        QVERIFY2(pSplitter->widget(1) == mpDialog->rightPanel, "the details column is not the splitter's second pane");
        QVERIFY2(!pSplitter->isCollapsible(0) && !pSplitter->isCollapsible(1), "a column can be dragged away to nothing, leaving the window with one of its two halves");
        QVERIFY2(qobject_cast<uiDesign::GripSplitterHandle*>(pSplitter->handle(1)), "the seam between the columns is a platform-drawn QSplitterHandle rather than the design's grip handle");
        QVERIFY2(!mpDialog->leftPanel->styleSheet().contains(qsl("border-right")), "the list column still draws the hairline seam that the handle is now, so the two are drawn one beside the other");

        // +40: QSplitter will not let a drag take a pane under its minimum width
        const int wanted = mpDialog->leftPanel->minimumWidth() + 40;
        const QList<int> before = pSplitter->sizes();
        pSplitter->setSizes({wanted, pSplitter->width() - wanted - uiDesign::GripSplitter::scmHandleThickness});
        TestPixels::settle();
        qInfo().noquote() << qsl("  the seam was moved to %1; the list column measures %2").arg(QString::number(wanted), QString::number(mpDialog->leftPanel->width()));
        QVERIFY2(std::abs(mpDialog->leftPanel->width() - wanted) <= 2,
                 qPrintable(qsl("the seam was set at %1 and the list column measures %2 - the columns are not the splitter's to size")
                                    .arg(QString::number(wanted), QString::number(mpDialog->leftPanel->width()))));
        pSplitter->setSizes(before);
        TestPixels::settle();

        // Windows of this case's own, so the fixture's stays open for later cases
        QSettings& settings = *MudletApp::getQSettings();
        settings.remove(qsl("packageManagerSplitterState"));

        QPointer<dlgPackageManager> pFirst = new dlgPackageManager(nullptr, mpHost);
        pFirst->show();
        QVERIFY(QTest::qWaitForWindowExposed(pFirst));
        TestPixels::settle();
        auto* pFirstSplit = pFirst->findChild<uiDesign::GripSplitter*>(qsl("packagesSplitter"));
        QVERIFY(pFirstSplit);
        const int dragged = pFirst->leftPanel->minimumWidth() + 60;
        pFirstSplit->setSizes({dragged, pFirstSplit->width() - dragged - uiDesign::GripSplitter::scmHandleThickness});
        TestPixels::settle();
        const int chosen = pFirst->leftPanel->width();
        pFirst->close();
        TestPixels::settle();
        QVERIFY2(!settings.value(qsl("packageManagerSplitterState")).toByteArray().isEmpty(),
                 "closing the window wrote nothing under 'packageManagerSplitterState', so the split the reader chose is gone with it");

        QPointer<dlgPackageManager> pSecond = new dlgPackageManager(nullptr, mpHost);
        pSecond->show();
        QVERIFY(QTest::qWaitForWindowExposed(pSecond));
        TestPixels::settle();
        const int restored = pSecond->leftPanel->width();
        qInfo().noquote() << qsl("  the seam was left at %1 and the next window opened at %2").arg(QString::number(chosen), QString::number(restored));
        QVERIFY2(std::abs(restored - chosen) <= 4,
                 qPrintable(qsl("the seam was left at %1 and the next window opened at %2 - the split is not put back").arg(QString::number(chosen), QString::number(restored))));
        pSecond->close();
        TestPixels::settle();
    }

    void test_theActionButtonsSayWhatTheyDo()
    {
        showView(NavigationView::Installed);
        mpDialog->packageList->setCurrentRow(0);
        TestPixels::settle();
        QCOMPARE(mpDialog->packageList->selectedItems().size(), 1);
        QVERIFY2(mpDialog->pushButton_remove->text() == qsl("Remove"),
                 qPrintable(qsl("with one package chosen Remove reads \"%1\" rather than the bare word").arg(mpDialog->pushButton_remove->text())));

        mpDialog->packageList->selectAll();
        TestPixels::settle();
        const int chosen = mpDialog->packageList->selectedItems().size();
        QVERIFY2(chosen >= 2, "SETUP: the Installed view lists fewer than two packages, so there is no plural to read");
        qInfo().noquote() << qsl("  with %1 packages chosen Remove reads \"%2\"").arg(QString::number(chosen), mpDialog->pushButton_remove->text());
        QVERIFY2(mpDialog->pushButton_remove->text() == qsl("Remove %1 packages").arg(chosen),
                 qPrintable(qsl("with %1 packages chosen Remove reads \"%2\" rather than \"Remove %1 packages\"").arg(QString::number(chosen), mpDialog->pushButton_remove->text())));

        showView(NavigationView::Explore);
        mpDialog->packageList->setCurrentRow(0);
        TestPixels::settle();
        QVERIFY2(mpDialog->pushButton_installRepo->text() == qsl("Install"),
                 qPrintable(qsl("with one package chosen Install reads \"%1\" rather than the bare word").arg(mpDialog->pushButton_installRepo->text())));
        mpDialog->packageList->selectAll();
        TestPixels::settle();
        const int many = mpDialog->packageList->selectedItems().size();
        QVERIFY2(many >= 2, "SETUP: the Explore view lists fewer than two packages, so there is no plural to read");
        qInfo().noquote() << qsl("  with %1 packages chosen Install reads \"%2\"").arg(QString::number(many), mpDialog->pushButton_installRepo->text());
        QVERIFY2(mpDialog->pushButton_installRepo->text() == qsl("Install %1 packages").arg(many),
                 qPrintable(qsl("with %1 packages chosen Install reads \"%2\" rather than \"Install %1 packages\"").arg(QString::number(many), mpDialog->pushButton_installRepo->text())));
        QVERIFY2(!mpDialog->pushButton_installRepo->toolTip().isEmpty(), "Install lost the tooltip saying what it fetches from");

        showView(NavigationView::Installed);
        mpDialog->packageList->setCurrentRow(0);
        TestPixels::settle();
    }

    void test_theWindowIsDraggedDownToTheTwoColumnsFloor()
    {
        // Listed at its installed version: Website and Report are only offered
        // for a package the repository knows, and the same version keeps it out
        // of the Updates view
        QVERIFY2(writeRepositoryIndex(qsl("1.2.3")), "SETUP: the repository index naming the installed package could not be written");
        QVERIFY2(mpDialog->readPackageRepositoryFile(), "SETUP: the repository index naming the installed package could not be read back");
        showView(NavigationView::Installed);
        mpDialog->packageList->setCurrentRow(rowOf(mFirstPackage));
        TestPixels::settle();
        QVERIFY2(!mpDialog->pushButton_website->isHidden(), "SETUP: the chosen package offers no Website button, so the row has nothing to give up");

        const QSize floor = mpDialog->minimumSizeHint();
        qInfo().noquote() << qsl("  the window's floor is %1x%2; the list column's is %3x%4 (minimum width %5), the details column's %6x%7 (minimum width %8)")
                                     .arg(QString::number(floor.width()),
                                          QString::number(floor.height()),
                                          QString::number(mpDialog->leftPanel->minimumSizeHint().width()),
                                          QString::number(mpDialog->leftPanel->minimumSizeHint().height()),
                                          QString::number(mpDialog->leftPanel->minimumWidth()))
                                     .arg(QString::number(mpDialog->rightPanel->minimumSizeHint().width()),
                                          QString::number(mpDialog->rightPanel->minimumSizeHint().height()),
                                          QString::number(mpDialog->rightPanel->minimumWidth()));
        QVERIFY2(floor.width() <= 700,
                 qPrintable(qsl("the window cannot be made narrower than %1px, where the two columns and the handle between them come to well under 700px - something in it is pinned open")
                                    .arg(QString::number(floor.width()))));
        QVERIFY2(floor.height() <= 450, qPrintable(qsl("the window cannot be made shorter than %1px, where nothing in either column asks for more than 450px").arg(QString::number(floor.height()))));

        const int narrow = floor.width();
        mpDialog->resize(narrow, 420);
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        QCoreApplication::processEvents();
        qInfo().noquote() << qsl("  asked for %1x420 the window measures %2x%3; the row needs %4px worded and has %5px; Website reads \"%6\"")
                                     .arg(QString::number(narrow),
                                          QString::number(mpDialog->width()),
                                          QString::number(mpDialog->height()),
                                          QString::number(mpDialog->actionRowWordedNeed()),
                                          QString::number(mpDialog->horizontalLayout_4->geometry().width()),
                                          mpDialog->pushButton_website->text());
        QVERIFY2(std::abs(mpDialog->width() - narrow) <= 4,
                 qPrintable(qsl("asked for %1px across the window measures %2px - it refused the size, so it cannot be dragged smaller than that either")
                                    .arg(QString::number(narrow), QString::number(mpDialog->width()))));
        QVERIFY2(mpDialog->actionRowWordedNeed() > mpDialog->horizontalLayout_4->geometry().width(),
                 qPrintable(qsl("SETUP: at the floor the row has %1px and needs only %2px with every word on it, so there is nothing for it to give up")
                                    .arg(QString::number(mpDialog->horizontalLayout_4->geometry().width()), QString::number(mpDialog->actionRowWordedNeed()))));
        QVERIFY2(mpDialog->pushButton_website->text().isEmpty(),
                 qPrintable(qsl("with the details column too narrow for the row's words Website still reads \"%1\", so the row is holding the window open").arg(mpDialog->pushButton_website->text())));
        QVERIFY2(!mpDialog->pushButton_website->icon().isNull(), "Website gave its word up and carries no glyph either, so it is an empty button");
        QVERIFY2(mpDialog->pushButton_website->accessibleName().contains(qsl("Website")),
                 qPrintable(qsl("standing as a glyph alone Website is announced as \"%1\", so a screen reader reads an unnamed button").arg(mpDialog->pushButton_website->accessibleName())));

        // The row's words come back at a wider width than they were taken away at,
        // so a width on the line does not flicker
        mpDialog->resize(1000, 600);
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        QCoreApplication::processEvents();
        qInfo().noquote() << qsl("  back at 1000px across Website reads \"%1\"").arg(mpDialog->pushButton_website->text());
        QVERIFY2(mpDialog->pushButton_website->text() == qsl("Website"),
                 qPrintable(qsl("with room for every word again Website reads \"%1\" rather than its own word").arg(mpDialog->pushButton_website->text())));

        mpDialog->resize(900, 600);
        QVERIFY(writeRepositoryIndex());
        QVERIFY(mpDialog->readPackageRepositoryFile());
        showView(NavigationView::Installed);
        mpDialog->packageList->setCurrentRow(0);
        TestPixels::settle();
    }

    // Nothing else restyles this window: no other window owns this QDialog, so
    // the appearance signal alone moves it
    void test_anAppearanceChangeRebuildsBothSheets()
    {
        QMap<QString, QString> sheets;
        for (const auto& appearance : QList<QPair<QString, enums::Appearance>>{{qsl("dark"), enums::Appearance::dark}, {qsl("light"), enums::Appearance::light}}) {
            TestPixels::setAppearanceAndWait(appearance.second, mpDialog);
            const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
            qInfo().noquote() << qsl("  %1: pane %2, page %3").arg(appearance.first, tokens.pane.name(), tokens.page.name());
            QVERIFY2(mpDialog->leftPanel->styleSheet().contains(tokens.pane.name()),
                     qPrintable(qsl("on the %1 appearance the list column's sheet does not name the pane tone %2 that appearance mixes").arg(appearance.first, tokens.pane.name())));
            QVERIFY2(mpDialog->rightPanel->styleSheet().contains(tokens.page.name()),
                     qPrintable(qsl("on the %1 appearance the details column's sheet does not name the page tone %2 that appearance mixes").arg(appearance.first, tokens.page.name())));
            QVERIFY2(!mpDialog->pushButton_installFile->icon().isNull(), qPrintable(qsl("on the %1 appearance Install from a file lost its glyph").arg(appearance.first)));
            sheets.insert(appearance.first, mpDialog->leftPanel->styleSheet());
        }
        QVERIFY2(sheets.value(qsl("dark")) != sheets.value(qsl("light")), "both appearances leave the list column with the same sheet, so the pass is not being re-run at all");
    }
};

#include "PackageManagerShellTest.moc"
MUDLET_GROUPED_TEST_MAIN(PackageManagerShellTest)
