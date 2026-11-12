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
 * The sidebar chevron the script editor and the settings dialog share: a window
 * too narrow for the names forces the rail without writing it to the window's
 * labels-shown key, and the chosen row reads bold at a width every name fits.
 *
 * Run with: ctest -R SidebarToggleTest -V
 */

#include <QColor>
#include <QComboBox>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QPixmap>
#include <QProxyStyle>
#include <QScopeGuard>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleOptionViewItem>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostDialogs.h"
#include "MudletApp.h"
#include "ProfileSessionTestHelper.h"
#include "SettingsTestHelper.h"
#include "SidebarToggle.h"
#include "dlgProfilePreferences.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"
#include "uiDesign.h"

#include "GroupedTest.h"

// On a desktop the appearances' styles leave different text margins (2px a side
// under the dark Fusion proxy, 4px under macOS's own), which decides elision;
// offscreen both are Fusion, so the difference is installed
namespace {
class WiderTextMarginStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;

    int pixelMetric(PixelMetric metric, const QStyleOption* pOption, const QWidget* pWidget) const override
    {
        const int given = QProxyStyle::pixelMetric(metric, pOption, pWidget);
        return metric == PM_FocusFrameHMargin ? given + 4 : given;
    }
};

// A bold chosen name measured 1.59x (settings) and 1.64x (editor) on macOS, and
// 1.42x and 1.35x with Windows' fonts; one weight in both states comes to 1.11x
// to 1.16x from the colours and the pill alone
constexpr double scmBoldInkFloor = 1.25;

// A solid pen's stems come back opaque; only antialiased edges are partial, and
// those follow the background rather than the weight
constexpr int scmOpaqueInk = 192;
constexpr int scmInkSpread = 24;

// A weight cannot be read back off a widget, so the picture is the only witness
int inkPixels(const QImage& picture, const QRect& area, const QColor& ink)
{
    int found = 0;
    const QRect inPicture = area.intersected(picture.rect());
    for (int y = inPicture.top(); y <= inPicture.bottom(); ++y) {
        for (int x = inPicture.left(); x <= inPicture.right(); ++x) {
            const QColor pixel = picture.pixelColor(x, y);
            if (pixel.alpha() < scmOpaqueInk) {
                continue;
            }
            const int spread = qMax(qMax(qAbs(pixel.red() - ink.red()), qAbs(pixel.green() - ink.green())), qAbs(pixel.blue() - ink.blue()));
            if (spread <= scmInkSpread) {
                ++found;
            }
        }
    }
    return found;
}
} // namespace

class SidebarToggleTest : public QObject
{
    Q_OBJECT

private:
    // A window as the shared cases read it: a list of named rows in a pane, the
    // chevron on the pane's seam, and a labels-shown preference of the window's own
    class Sidebar
    {
    public:
        virtual ~Sidebar() = default;
        virtual QWidget* window() const = 0;
        // The chevron's parent, so the frame both it and the seam are measured in
        virtual QWidget* shell() const = 0;
        virtual QWidget* pane() const = 0;
        virtual QListWidget* list() const = 0;
        virtual uiDesign::SidebarToggle* toggle() const = 0;
        virtual QString preferenceKey() const = 0;
        // Where the window keeps the choice: the editor in a member it writes out
        // on close, the settings in QSettings as it is made
        virtual bool preferenceSaysNames() const = 0;
        // The item colour each window hands sidebarStyleSheet()
        virtual QColor unchosenInk(const uiDesign::ThemeTokens& tokens) const = 0;
        virtual void resizeWide() = 0;
        virtual void resizeNarrow() = 0;
        // The breakpoint moves with the style, and a window left too narrow would
        // answer with a rail
        virtual void resizeWideUnderThisStyle() { resizeWide(); }

        // The property the delegate and the sidebar's rules read, so it is what is
        // drawn rather than a copy of the intent
        bool railShowing() const { return list()->property(uiDesign::scmProp_rail).toBool(); }
        bool storedLabelsShown() const { return MudletApp::getQSettings()->value(preferenceKey(), true).toBool(); }
        bool anythingStored() const { return MudletApp::getQSettings()->contains(preferenceKey()); }

        QString state() const
        {
            return qsl("window %1, sidebar %2, rail %3, stored labelsShown %4")
                    .arg(QString::number(window()->width()),
                         QString::number(pane()->width()),
                         railShowing() ? qsl("yes") : qsl("no"),
                         anythingStored() ? (storedLabelsShown() ? qsl("true") : qsl("false")) : qsl("nothing"));
        }

    protected:
        static void settle(QWidget* pWindow, const QSize& size)
        {
            pWindow->resize(size);
            QCoreApplication::sendPostedEvents();
        }
    };

    class EditorSidebar : public Sidebar
    {
    public:
        dlgTriggerEditor* mpEditor = nullptr;
        // Measured off the editor once shown: the breakpoint is the widest
        // translated name plus the narrowest body, neither of which is a number in
        // the source
        int mWideEnough = 0;
        int mTooNarrow = 0;
        static constexpr int scmHeight = 700;

        QWidget* window() const override { return mpEditor; }
        QWidget* shell() const override { return mpEditor->centralWidget(); }
        QWidget* pane() const override { return mpEditor->mpWidget_editorSidebarPane; }
        QListWidget* list() const override { return mpEditor->mpListWidget_editorSidebar; }
        uiDesign::SidebarToggle* toggle() const override { return mpEditor->mpToggle_editorSidebar; }
        QString preferenceKey() const override { return qsl("editorSidebarLabelsShown"); }
        bool preferenceSaysNames() const override { return mpEditor->mEditorSidebarLabelsShown; }
        QColor unchosenInk(const uiDesign::ThemeTokens& tokens) const override { return tokens.mutedText; }
        void resizeWide() override { resizeTo(mWideEnough); }
        void resizeNarrow() override { resizeTo(mTooNarrow); }
        void resizeWideUnderThisStyle() override { resizeTo(widthWithRoomForTheNames()); }

        int widthWithRoomForTheNames() const { return mpEditor->editorSidebarWidths().collapseBelow + 120; }

        void resizeTo(const int width) { settle(mpEditor, QSize(width, scmHeight)); }

        void open(Host* pHost)
        {
            mudlet::self()->slot_showEditorDialog();
            QCoreApplication::processEvents();
            mpEditor = HostDialogs::of(pHost).mpEditorDialog;
        }
    };

    class SettingsSidebar : public Sidebar
    {
    public:
        Host* mpHost = nullptr;
        dlgProfilePreferences* mpPreferences = nullptr;
        // The dialog's minimum size, well under the width the names need
        const QSize mTooNarrow = QSize(780, 560);
        const QSize mWideEnough = QSize(1060, 760);

        QWidget* window() const override { return mpPreferences; }
        QWidget* shell() const override { return mpPreferences->findChild<QWidget*>(qsl("settingsShell")); }
        QWidget* pane() const override { return mpPreferences->findChild<QWidget*>(qsl("settingsSidebar")); }
        QListWidget* list() const override { return TestSettings::sidebar(mpPreferences); }
        uiDesign::SidebarToggle* toggle() const override { return mpPreferences->findChild<uiDesign::SidebarToggle*>(qsl("settingsSidebarToggle")); }
        QString preferenceKey() const override { return qsl("settingsSidebarLabelsShown"); }
        bool preferenceSaysNames() const override { return storedLabelsShown(); }
        QColor unchosenInk(const uiDesign::ThemeTokens& tokens) const override { return tokens.text; }
        void resizeWide() override { settle(mpPreferences, mWideEnough); }
        void resizeNarrow() override { settle(mpPreferences, mTooNarrow); }

        // The dialog's own control: the path that restyles every open window, not
        // only this one
        void takeTo(const enums::Appearance appearance)
        {
            if (mpPreferences->comboBox_appearance->currentIndex() == appearance) {
                mpPreferences->comboBox_appearance->setCurrentIndex(appearance == enums::Appearance::dark ? enums::Appearance::light : enums::Appearance::dark);
                QCoreApplication::sendPostedEvents();
            }
            mpPreferences->comboBox_appearance->setCurrentIndex(appearance);
            QCoreApplication::sendPostedEvents();
        }

        void open()
        {
            mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
            mpPreferences->resize(mWideEnough);
            mpPreferences->show();
            QVERIFY(QTest::qWaitForWindowExposed(mpPreferences));
            QVERIFY2(list(), "the settings shell has no category sidebar");
            QVERIFY2(toggle(), "the settings sidebar has no chevron on its seam");
        }

        void close()
        {
            delete mpPreferences;
            mpPreferences = nullptr;
        }

        QLabel* wordmark() const { return mpPreferences->findChild<QLabel*>(qsl("settingsWordmark")); }

        QListWidgetItem* mapperRow() const { return list()->item(TestSettings::rowOf(mpPreferences, qsl("mapper"))); }

        int widestBoldName() const
        {
            QFont nameFont = list()->font();
            nameFont.setBold(true);
            const QFontMetrics nameMetrics(nameFont);
            int widest = 0;
            for (int row = 0, rows = list()->count(); row < rows; ++row) {
                widest = std::max(widest, nameMetrics.horizontalAdvance(list()->item(row)->text()));
            }
            return widest;
        }
    };

    TestProfile::Session mSession;
    QByteArray mSavedNoThemeDownload;
    Host* mpHost = nullptr;
    EditorSidebar mEditor;
    SettingsSidebar mSettings;
    const QString mProfileName = qsl("SidebarToggle-Test-Profile");

    Sidebar& sidebarNamed(const QString& window) { return window == qsl("editor") ? static_cast<Sidebar&>(mEditor) : static_cast<Sidebar&>(mSettings); }

    static void addBothWindows()
    {
        QTest::addColumn<QString>("window");
        QTest::newRow("editor") << qsl("editor");
        QTest::newRow("settings") << qsl("settings");
    }

    void pressTheToggle(const Sidebar& sidebar)
    {
        QVERIFY2(sidebar.toggle() != nullptr, "the window has no sidebar toggle");
        QVERIFY2(sidebar.toggle()->isEnabled(), "the sidebar toggle is not pressable at this width");
        QTest::mouseClick(sidebar.toggle(), Qt::LeftButton);
        QCoreApplication::sendPostedEvents();
    }

    // In bold, as a chosen row is drawn, so the same rect covers a name in either
    // state. Any picture of the right size: a row reserves the glyph's space, not
    // the glyph.
    static QStyleOptionViewItem boldRowOption(const QListWidget* pList)
    {
        QFont nameFont = pList->font();
        nameFont.setBold(true);

        QStyleOptionViewItem option;
        option.initFrom(pList);
        option.font = nameFont;
        option.fontMetrics = QFontMetrics(nameFont);
        option.features |= QStyleOptionViewItem::HasDisplay | QStyleOptionViewItem::HasDecoration;
        option.decorationSize = pList->iconSize();
        option.decorationPosition = QStyleOptionViewItem::Left;
        option.displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
        QPixmap blank(pList->iconSize());
        blank.fill(Qt::transparent);
        option.icon = QIcon(blank);
        return option;
    }

    // QCommonStyle draws inside the text rect shrunk by PM_FocusFrameHMargin + 1 a
    // side, so a name that just fits it is elided
    static QStringList namesTooNarrowForTheirRows(const Sidebar& sidebar, QStringList& measured)
    {
        QListWidget* pList = sidebar.list();
        QStyleOptionViewItem option = boldRowOption(pList);
        const QFontMetrics nameMetrics = option.fontMetrics;

        const int drawingMargin = 2 * (pList->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, &option, pList) + 1);
        QStringList squeezed;
        int tightest = 0;
        QString tightestName;
        for (int row = 0, rows = pList->count(); row < rows; ++row) {
            QListWidgetItem* pItem = pList->item(row);
            // Divider rows have no name; the editor's Folders row hides while the
            // profile lacks that view
            if (pItem->text().isEmpty() || pItem->isHidden()) {
                continue;
            }
            option.text = pItem->text();
            option.rect = pList->visualItemRect(pItem);
            const int room = pList->style()->subElementRect(QStyle::SE_ItemViewItemText, &option, pList).width() - drawingMargin;
            const int needed = nameMetrics.horizontalAdvance(pItem->text());
            if (tightestName.isEmpty() || room - needed < tightest) {
                tightest = room - needed;
                tightestName = pItem->text();
            }
            if (room < needed) {
                squeezed << qsl("\"%1\" has %2px of a row %3px wide for a name %4px across").arg(pItem->text(), QString::number(room), QString::number(option.rect.width()), QString::number(needed));
            }
        }
        measured << qsl("sidebar %1px, style %2, %3px of margin, tightest row \"%4\" with %5px to spare")
                            .arg(QString::number(sidebar.pane()->width()),
                                 QString::number(pList->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, &option, pList)),
                                 QString::number(drawingMargin),
                                 tightestName,
                                 QString::number(tightest));
        return squeezed;
    }

    static QRect nameAreaOf(const Sidebar& sidebar, const int row)
    {
        QListWidget* pList = sidebar.list();
        QStyleOptionViewItem option = boldRowOption(pList);
        option.text = pList->item(row)->text();
        option.rect = pList->visualItemRect(pList->item(row));
        // visualItemRect answers in the viewport's coordinates and grab() takes
        // the whole list, frame included
        return pList->style()->subElementRect(QStyle::SE_ItemViewItemText, &option, pList).translated(pList->viewport()->mapTo(pList, QPoint(0, 0)));
    }

    // Dividers have no name, the editor's Folders row hides while the profile
    // lacks that view, and the support link at the foot of the settings is named
    // but never selectable
    static bool aChoosableNamedRow(const QListWidgetItem* pItem) { return !pItem->text().isEmpty() && !pItem->isHidden() && (pItem->flags() & Qt::ItemIsSelectable); }

    // The longest name shows a change of weight most
    static int longestNamedRow(const Sidebar& sidebar)
    {
        QListWidget* pList = sidebar.list();
        int longest = -1;
        for (int row = 0, rows = pList->count(); row < rows; ++row) {
            if (!aChoosableNamedRow(pList->item(row))) {
                continue;
            }
            if (longest < 0 || pList->item(row)->text().length() > pList->item(longest)->text().length()) {
                longest = row;
            }
        }
        return longest;
    }

    static int aNamedRowOtherThan(const Sidebar& sidebar, const int row)
    {
        QListWidget* pList = sidebar.list();
        for (int other = 0, rows = pList->count(); other < rows; ++other) {
            if (other != row && aChoosableNamedRow(pList->item(other))) {
                return other;
            }
        }
        return -1;
    }

    static void chooseRow(const Sidebar& sidebar, const int row)
    {
        sidebar.list()->setCurrentRow(row);
        QCoreApplication::sendPostedEvents();
    }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        mSavedNoThemeDownload = qgetenv("MUDLET_TEST_NO_THEME_DOWNLOAD");
        qputenv("MUDLET_TEST_NO_THEME_DOWNLOAD", "1");

        QVERIFY2(mSession.start(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();
        QVERIFY2(!mSettings.anythingStored(), "the sidebar preference was already written before a dialog was ever opened, so the default below proves nothing");

        mEditor.open(mpHost);
        QVERIFY2(mEditor.mpEditor != nullptr, "Editor dialog should be created");

        // Only measurable once shown: the breakpoint comes from splitter_main's
        // minimum size hint, which is stale until its layout has run
        const dlgTriggerEditor::EditorSidebarWidths widths = mEditor.mpEditor->editorSidebarWidths();
        mEditor.mWideEnough = widths.collapseBelow + 120;
        mEditor.mTooNarrow = widths.collapseBelow - 40;
        qInfo().noquote() << qsl("  editor: sidebar expanded at %1px, names given up below %2px; editor minimum width %3px")
                                     .arg(QString::number(widths.expanded), QString::number(widths.collapseBelow), QString::number(mEditor.mpEditor->minimumWidth()));
        // The layout's minimum is the sidebar's fixed width plus the body's, so a
        // breakpoint at or below it is one no drag can reach
        QVERIFY2(mEditor.mTooNarrow >= mEditor.mpEditor->minimumWidth(),
                 qPrintable(qsl("The editor cannot be dragged below its own breakpoint (minimum %1px, breakpoint %2px), so the forced rail is unreachable")
                                    .arg(QString::number(mEditor.mpEditor->minimumWidth()), QString::number(widths.collapseBelow))));
        mEditor.resizeWide();

        mSettings.mpHost = mpHost;
        mSettings.open();
    }

    void cleanupTestCase()
    {
        mSettings.close();
        mEditor.mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
        mSavedNoThemeDownload.isNull() ? qunsetenv("MUDLET_TEST_NO_THEME_DOWNLOAD") : qputenv("MUDLET_TEST_NO_THEME_DOWNLOAD", mSavedNoThemeDownload);
    }

    void test_aWindowNobodyHasToldOpensWithItsNames_data() { addBothWindows(); }

    void test_aWindowNobodyHasToldOpensWithItsNames()
    {
        QFETCH(QString, window);
        const Sidebar& sidebar = sidebarNamed(window);
        qInfo().noquote() << qsl("  %1").arg(sidebar.state());
        QVERIFY2(!sidebar.railShowing(), qPrintable(qsl("the window opened as a rail: %1").arg(sidebar.state())));
        // A sidebar that kept the rail's width while its property said otherwise
        // would pass on the property alone
        QVERIFY2(sidebar.pane()->width() > uiDesign::scmSidebarRailWidth,
                 qPrintable(qsl("the sidebar is at the rail's %1px with its names showing: %2").arg(QString::number(uiDesign::scmSidebarRailWidth), sidebar.state())));
        QVERIFY2(!sidebar.anythingStored(), "opening the window wrote a sidebar preference nobody chose, so what a later run opens with is not the default this case reads");
        QVERIFY2(sidebar.toggle() != nullptr, "the window has no sidebar toggle");
        QVERIFY2(sidebar.toggle()->isEnabled(), "the chevron cannot be pressed on a window with room for the names");
        QVERIFY2(sidebar.toggle()->pointingLeft(), "the names are showing and the chevron does not point at the sidebar");
    }

    void test_theToggleRidesTheSeamBesideTheSidebar_data() { addBothWindows(); }

    // Read in both modes: the seam moves when the sidebar gives its names up
    void test_theToggleRidesTheSeamBesideTheSidebar()
    {
        QFETCH(QString, window);
        const Sidebar& sidebar = sidebarNamed(window);
        QVERIFY2(sidebar.toggle() != nullptr, "the window has no sidebar toggle");
        QVERIFY2(sidebar.toggle()->parentWidget() == sidebar.shell(), "the toggle is a child of one of the two things it lies between, which would clip it");

        QWidget* pShell = sidebar.shell();
        QWidget* pPane = sidebar.pane();
        uiDesign::SidebarToggle* pToggle = sidebar.toggle();
        QStringList measured;
        QStringList adrift;
        for (const QString& mode : {qsl("with the names showing"), qsl("as a rail")}) {
            const QPoint paneTopLeft = pPane->mapTo(pShell, QPoint(0, 0));
            const int seamX = paneTopLeft.x() + pPane->width();
            const int fromTheSeam = qRound(pToggle->x() + pToggle->width() / 2.0) - seamX;
            const int paneMiddle = paneTopLeft.y() + pPane->height() / 2;
            const int fromTheMiddle = qRound(pToggle->y() + pToggle->height() / 2.0) - paneMiddle;
            measured << qsl("%1: the sidebar ends at x=%2 and the chevron's middle is %3px off it, %4px off the pane's own middle")
                                .arg(mode, QString::number(seamX), QString::number(fromTheSeam), QString::number(fromTheMiddle));
            // A pixel either way: the pill straddles a line rather than filling
            // a column, so its middle lands on the boundary between two pixels
            if (std::abs(fromTheSeam) > 1) {
                adrift << qsl("%1, %2px off the seam").arg(mode, QString::number(fromTheSeam));
            }
            if (std::abs(fromTheMiddle) > 1) {
                adrift << qsl("%1, %2px off the middle of the pane").arg(mode, QString::number(fromTheMiddle));
            }
            pressTheToggle(sidebar);
        }
        qInfo().noquote() << qsl("  %1").arg(measured.join(qsl("; ")));

        QVERIFY2(adrift.isEmpty(), qPrintable(qsl("the sidebar toggle is not on the seam: %1").arg(adrift.join(qsl("; ")))));
        // The next case starts from the names showing
        QVERIFY2(!sidebar.railShowing(), qPrintable(qsl("the second press did not put the names back: %1").arg(sidebar.state())));
    }

    // Read off the picture as well as the flag, so a chevron that stopped
    // following the mode is caught
    void test_theChevronTurnsRoundWithTheMode()
    {
        QVERIFY2(!mEditor.railShowing(), "This case starts from the names showing");
        QVERIFY2(mEditor.toggle()->pointingLeft(), "The names are showing and the chevron does not point at the sidebar");
        const QImage pointingIn = mEditor.toggle()->grab().toImage();

        pressTheToggle(mEditor);
        QVERIFY2(mEditor.railShowing(), "The toggle did not minimise the sidebar");
        QVERIFY2(!mEditor.toggle()->pointingLeft(), "The sidebar is a rail and the chevron still points at it");
        const QImage pointingOut = mEditor.toggle()->grab().toImage();
        qInfo().noquote() << qsl("  the chevron is %1px by %2px and is drawn two ways").arg(QString::number(mEditor.toggle()->width()), QString::number(mEditor.toggle()->height()));
        QVERIFY2(pointingIn != pointingOut, "The chevron is drawn the same way in both of the sidebar's modes");

        pressTheToggle(mEditor);
        QVERIFY2(!mEditor.railShowing() && mEditor.toggle()->pointingLeft(), "The toggle did not turn back round with the names");
    }

    // The rail keeps each row's text, which a screen reader announces, and shows
    // it on hover
    void test_theToggleTakesTheNamesAwayAndWritesItDown()
    {
        QVERIFY2(!mSettings.railShowing(), "this case starts from the names showing");
        QListWidgetItem* pItem = mSettings.mapperRow();
        QVERIFY2(!pItem->text().isEmpty(), "a row lost the text that is its accessible name");
        QVERIFY2(pItem->toolTip().isEmpty(), "a row with its name on show still repeats it as a tooltip");
        QVERIFY2(!mSettings.wordmark()->isHidden(), "the wordmark is hidden beside a sidebar with room for it");

        pressTheToggle(mSettings);
        qInfo().noquote() << qsl("  %1").arg(mSettings.state());
        QVERIFY2(mSettings.railShowing(), qPrintable(qsl("the toggle did not take the names away: %1").arg(mSettings.state())));
        QCOMPARE(mSettings.pane()->width(), uiDesign::scmSidebarRailWidth);
        QVERIFY2(mSettings.anythingStored() && !mSettings.storedLabelsShown(), qPrintable(qsl("pressing the toggle did not write the choice down: %1").arg(mSettings.state())));
        QVERIFY2(!mSettings.toggle()->pointingLeft(), "the sidebar is a rail and the chevron still points into it");
        QCOMPARE(pItem->toolTip(), pItem->text());
        QVERIFY2(mSettings.wordmark()->isHidden(), "the wordmark is drawn beside a rail that has no room for it");

        // Back to the names, where the cases below start
        pressTheToggle(mSettings);
        QVERIFY2(!mSettings.railShowing(), qPrintable(qsl("the second press did not put the names back: %1").arg(mSettings.state())));
        QVERIFY2(mSettings.storedLabelsShown(), qPrintable(qsl("putting the names back did not write the choice down: %1").arg(mSettings.state())));
    }

    void test_theSidebarIsAsWideAsItsNamesAndNoWider()
    {
        QVERIFY2(!mSettings.railShowing(), "this case starts from the names showing");
        const int measured = mSettings.pane()->width();
        const int widest = mSettings.widestBoldName();
        qInfo().noquote() << qsl("  the sidebar is %1px wide for a widest bold name of %2px").arg(QString::number(measured), QString::number(widest));
        QVERIFY2(measured >= widest + uiDesign::scmSidebarRailWidth,
                 qPrintable(qsl("the sidebar is %1px wide, which leaves the widest name (%2px) less than a rail's width of chrome around it").arg(measured).arg(widest)));
        QVERIFY2(measured <= 232, qPrintable(qsl("the measured sidebar is %1px, wider than the flat 232px it replaced").arg(measured)));
        QWidget* pWordmarkRow = mSettings.wordmark()->parentWidget();
        QVERIFY2(pWordmarkRow->width() >= pWordmarkRow->sizeHint().width(),
                 qPrintable(qsl("the wordmark row wants %1px and was given %2px").arg(pWordmarkRow->sizeHint().width()).arg(pWordmarkRow->width())));
    }

    void test_aNarrowWindowForcesTheRailWithoutStoringIt_data() { addBothWindows(); }

    void test_aNarrowWindowForcesTheRailWithoutStoringIt()
    {
        QFETCH(QString, window);
        Sidebar& sidebar = sidebarNamed(window);
        QVERIFY2(!sidebar.railShowing(), "this case starts from the names showing");

        sidebar.resizeNarrow();
        qInfo().noquote() << qsl("  %1").arg(sidebar.state());
        QVERIFY2(sidebar.railShowing(), qPrintable(qsl("a window too narrow for the names kept them: %1").arg(sidebar.state())));
        QVERIFY2(sidebar.preferenceSaysNames(), qPrintable(qsl("running out of room rewrote the user's preference: %1").arg(sidebar.state())));
        QVERIFY2(!sidebar.toggle()->isEnabled(), "the toggle offers labels a window this narrow cannot draw");
        QVERIFY2(sidebar.toggle()->toolTip() != sidebar.toggle()->accessibleName(), "the toggle a narrow window has disabled still offers to show the labels");

        sidebar.resizeWide();
        qInfo().noquote() << qsl("  %1").arg(sidebar.state());
        QVERIFY2(!sidebar.railShowing(), qPrintable(qsl("the names did not come back when the window had room again: %1").arg(sidebar.state())));
        QVERIFY2(sidebar.toggle()->isEnabled(), "the toggle is still refusing on a window with room for the names");
    }

    void test_theToggleNamesWhatItDoesToTheLabels_data() { addBothWindows(); }

    void test_theToggleNamesWhatItDoesToTheLabels()
    {
        QFETCH(QString, window);
        const Sidebar& sidebar = sidebarNamed(window);
        const QString showing = sidebar.toggle()->accessibleName();
        QVERIFY2(!showing.isEmpty(), "the sidebar toggle has no accessible name");
        pressTheToggle(sidebar);
        const QString rail = sidebar.toggle()->accessibleName();
        qInfo().noquote() << qsl("  with the names showing: \"%1\"; as a rail: \"%2\"").arg(showing, rail);
        QVERIFY2(rail != showing, "the sidebar toggle says the same thing in both modes");

        // The sidebar is always there, so no wording may be about opening or closing it
        for (const QString& wording : {showing, rail}) {
            QVERIFY2(!wording.contains(qsl("close"), Qt::CaseInsensitive) && !wording.contains(qsl("open"), Qt::CaseInsensitive),
                     qPrintable(qsl("the sidebar toggle reads as opening or closing the sidebar: \"%1\"").arg(wording)));
        }
    }

    // Starts from the rail the previous case's press left
    void test_aManualRailStaysOnAWideWindow()
    {
        QVERIFY2(mEditor.railShowing(), qPrintable(qsl("The toggle did not minimise the sidebar: %1").arg(mEditor.state())));

        mEditor.resizeTo(mEditor.mWideEnough + 200);
        QVERIFY2(mEditor.railShowing(), qPrintable(qsl("A wider window put the names back over the user's choice: %1").arg(mEditor.state())));
        mEditor.resizeWide();
    }

    void test_theTogglePutsTheNamesBack()
    {
        pressTheToggle(mEditor);
        QVERIFY2(!mEditor.railShowing(), qPrintable(qsl("The toggle did not bring the names back: %1").arg(mEditor.state())));
    }

    // mEditorSidebarLabelsShown is what a close writes out
    void test_aManualRailSurvivesAFreshEditor()
    {
        pressTheToggle(mEditor);
        QVERIFY2(mEditor.railShowing(), "The toggle did not minimise the sidebar");

        mEditor.mpEditor->close();
        QCoreApplication::processEvents();
        QVERIFY2(!mEditor.storedLabelsShown(), "Closing the editor did not write the toggle's choice down");

        delete mEditor.mpEditor;
        mEditor.mpEditor = nullptr;
        QVERIFY2(HostDialogs::of(mpHost).mpEditorDialog.isNull(), "The editor should have been destroyed");

        mEditor.open(mpHost);
        QVERIFY2(mEditor.mpEditor != nullptr, "A fresh editor should have been created");
        mEditor.resizeWide();

        qInfo().noquote() << qsl("  %1").arg(mEditor.state());
        QVERIFY2(mEditor.railShowing(), qPrintable(qsl("A fresh editor forgot that the names had been given up: %1").arg(mEditor.state())));
    }

    void test_theChoiceSurvivesAFreshDialog()
    {
        // The press in test_theToggleNamesWhatItDoesToTheLabels is the last thing
        // to have touched the settings' preference
        QVERIFY2(!mSettings.storedLabelsShown(), qPrintable(qsl("the press above did not write the rail down: %1").arg(mSettings.state())));
        QVERIFY2(mSettings.railShowing(), "this case starts from the rail the case above left");
        mSettings.close();
        mSettings.open();
        QVERIFY2(mSettings.railShowing(), qPrintable(qsl("a fresh dialog forgot that the names had been given up: %1").arg(mSettings.state())));

        pressTheToggle(mSettings);
        QVERIFY2(!mSettings.railShowing(), "the toggle did not put the names up");
        mSettings.close();
        mSettings.open();
        qInfo().noquote() << qsl("  %1").arg(mSettings.state());
        QVERIFY2(!mSettings.railShowing(), qPrintable(qsl("a fresh dialog forgot that the names had been asked for: %1").arg(mSettings.state())));
    }

    void test_theChosenRowIsDrawnBold_data() { addBothWindows(); }

    // sidebarRowWidth() measures names in bold, so a regular chosen row means a
    // pane wider than it needs. A font-weight on ::item never reaches the painter,
    // which uses the style option's font, so the weight is SidebarItemDelegate's
    // and only the picture can prove it.
    void test_theChosenRowIsDrawnBold()
    {
        QFETCH(QString, window);
        Sidebar& sidebar = sidebarNamed(window);
        if (sidebar.railShowing()) {
            pressTheToggle(sidebar);
        }
        QVERIFY2(!sidebar.railShowing(), qPrintable(qsl("this case needs the names showing: %1").arg(sidebar.state())));
        sidebar.resizeWide();

        QListWidget* pList = sidebar.list();
        const int chosen = longestNamedRow(sidebar);
        const int other = aNamedRowOtherThan(sidebar, chosen);
        QVERIFY2(chosen >= 0 && other >= 0, "the sidebar has too few named rows to tell a chosen one from the rest");
        const QString name = pList->item(chosen)->text();
        const QRect nameArea = nameAreaOf(sidebar, chosen);

        // A width that changed with the selection would shift every row under
        // the pointer
        const int paneBefore = sidebar.pane()->width();
        const int rowBefore = pList->visualItemRect(pList->item(chosen)).width();

        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        chooseRow(sidebar, chosen);
        QCOMPARE(pList->currentRow(), chosen);
        const int inkWhenChosen = inkPixels(pList->grab().toImage(), nameArea, tokens.accentText);
        const int paneChosen = sidebar.pane()->width();
        const int rowChosen = pList->visualItemRect(pList->item(chosen)).width();

        chooseRow(sidebar, other);
        QCOMPARE(pList->currentRow(), other);
        const int inkWhenNot = inkPixels(pList->grab().toImage(), nameArea, sidebar.unchosenInk(tokens));

        const double ratio = inkWhenNot > 0 ? static_cast<double>(inkWhenChosen) / inkWhenNot : 0.0;
        qInfo().noquote() << qsl("  \"%1\" is inked with %2px chosen against %3px unchosen (%4x, floor %5x); pane %6px throughout, row %7px")
                                     .arg(name,
                                          QString::number(inkWhenChosen),
                                          QString::number(inkWhenNot),
                                          QString::number(ratio, 'f', 2),
                                          QString::number(scmBoldInkFloor, 'f', 2),
                                          QString::number(paneBefore),
                                          QString::number(rowChosen));

        QVERIFY2(inkWhenChosen > 0 && inkWhenNot > 0,
                 qPrintable(qsl("\"%1\" was read in a %2x%3 rect at %4,%5 and holds no ink in either state (%6px chosen, %7px unchosen), so the two pictures say nothing")
                                    .arg(name,
                                         QString::number(nameArea.width()),
                                         QString::number(nameArea.height()),
                                         QString::number(nameArea.x()),
                                         QString::number(nameArea.y()),
                                         QString::number(inkWhenChosen),
                                         QString::number(inkWhenNot))));
        QVERIFY2(ratio >= scmBoldInkFloor,
                 qPrintable(
                         qsl("\"%1\" is drawn with %2px of ink when it is the chosen row and %3px when it is not - %4x, under the %5x a bold name comes to, so the chosen row is not being drawn bold")
                                 .arg(name, QString::number(inkWhenChosen), QString::number(inkWhenNot), QString::number(ratio, 'f', 2), QString::number(scmBoldInkFloor, 'f', 2))));
        QVERIFY2(paneChosen == paneBefore && rowChosen == rowBefore,
                 qPrintable(qsl("choosing \"%1\" moved the sidebar: pane %2px to %3px, row %4px to %5px")
                                    .arg(name, QString::number(paneBefore), QString::number(paneChosen), QString::number(rowBefore), QString::number(rowChosen))));
    }

    // Both windows under each style, so each style is put on once
    void test_everyNameFitsItsRowUnderTheStyleDrawingThem()
    {
        const auto appearanceBefore = mudlet::self()->mAppearance;
        auto restore = qScopeGuard([appearanceBefore]() {
            // The loading flag makes setAppearance() rebuild its own style even
            // for the appearance already in force, replacing WiderTextMarginStyle
            mudlet::self()->setAppearance(appearanceBefore, true);
        });
        const QList<QPair<QString, Sidebar*>> sidebars{{qsl("editor"), &mEditor}, {qsl("settings"), &mSettings}};
        for (const auto& [window, pSidebar] : sidebars) {
            if (pSidebar->railShowing()) {
                pressTheToggle(*pSidebar);
            }
            QVERIFY2(!pSidebar->railShowing(), qPrintable(qsl("%1: this case needs the names showing: %2").arg(window, pSidebar->state())));
        }

        QStringList squeezed;
        const auto walkTheRows = [&sidebars, &squeezed](const QString& drawnBy) {
            QCoreApplication::sendPostedEvents();
            for (const auto& [window, pSidebar] : sidebars) {
                pSidebar->resizeWideUnderThisStyle();
                // Reported, not asserted: QVERIFY2 in a lambda only returns from it,
                // leaving the case passing on an empty list
                if (pSidebar->railShowing()) {
                    squeezed << qsl("%1, %2: the sidebar gave its names up altogether (%3)").arg(window, drawnBy, pSidebar->state());
                    continue;
                }

                QStringList measured;
                const QStringList tooNarrow = namesTooNarrowForTheirRows(*pSidebar, measured);
                qInfo().noquote() << qsl("  %1, %2: %3").arg(window, drawnBy, measured.join(qsl("; ")));
                for (const QString& complaint : tooNarrow) {
                    squeezed << qsl("%1, %2: %3").arg(window, drawnBy, complaint);
                }
            }
        };

        // Through the settings dialog's control, whose restyle reaches the editor
        // too by the same setAppearance() the editor's own path calls
        for (const auto& appearance : QList<QPair<QString, enums::Appearance>>{{qsl("dark"), enums::Appearance::dark}, {qsl("light"), enums::Appearance::light}}) {
            mSettings.takeTo(appearance.second);
            walkTheRows(appearance.first);
        }

        qApp->setStyle(new WiderTextMarginStyle(QStyleFactory::create(qsl("Fusion"))));
        walkTheRows(qsl("a style with wider text margins"));

        QVERIFY2(squeezed.isEmpty(), qPrintable(qsl("the sidebar does not hold its names under every style: %1").arg(squeezed.join(qsl("; ")))));
    }
};

#include "SidebarToggleTest.moc"
MUDLET_GROUPED_TEST_MAIN(SidebarToggleTest)
