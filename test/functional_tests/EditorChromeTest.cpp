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
 * The script editor's chrome as a shot of the window shows it: every word outside
 * a field in the one quiet ink, each column its own surface, seams and headings
 * cut to the design's shapes, and both menus drawn by the design language.
 *
 * Run with: ctest -R EditorChromeTest -V
 */

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QAction>
#include <QComboBox>
#include <QGroupBox>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPalette>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScopeGuard>
#include <QSplitterHandle>
#include <QStyleOptionToolButton>
#include <QTextEdit>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QtTest/QtTest>
#include <chrono>
#include <cstdlib>

#include "ChipRow.h"
#include "GripSplitter.h"
#include "Host.h"
#include "HostDialogs.h"
#include "MudletApp.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "dlgProfilePreferences.h"
#include "dlgTriggerEditor.h"
#include "dlgTriggerPatternEdit.h"
#include "dlgTriggersMainArea.h"
#include "mudlet.h"
#include "uiDesign.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorChromeTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    Host* mpHost = nullptr;
    enums::Appearance mAppearanceAtStart = enums::Appearance::systemSetting;
    const QString mProfileName = qsl("EditorChrome-Test-Profile");

    // Pinned under the measured 127 (dark) and 133 (light), so a walk that stops
    // finding widgets cannot pass as one that found them all in the one ink
    static constexpr int scmLeastAudited = 100;
    // Small: a glyph's antialiased edges step between ink and surface, so the
    // pixel furthest from the surface is the ink itself
    static constexpr int scmInkTolerance = 2;

    // The compile and duplicate notes say a reading in the error hue, and a chip
    // label holds an event name the reader typed - content, not chrome
    static bool aStateChipOrAChipsOwnName(const QWidget* pWidget)
    {
        for (const QWidget* pAt = pWidget; pAt; pAt = pAt->parentWidget()) {
            const QString name = pAt->objectName();
            if (name == qsl("editorCompileNote") || name == qsl("editorChipNote") || name == qsl("editorChipLabel")) {
                return true;
            }
        }
        return false;
    }

    // Fields keep the full tone; edbee draws from its syntax theme and the error
    // console in the colours the profile chose
    static bool insideAField(const QWidget* pWidget)
    {
        for (const QWidget* pAt = pWidget; pAt; pAt = pAt->parentWidget()) {
            if (qobject_cast<const QLineEdit*>(pAt) || qobject_cast<const QAbstractSpinBox*>(pAt) || qobject_cast<const QComboBox*>(pAt) || qobject_cast<const QTextEdit*>(pAt)
                || qobject_cast<const QPlainTextEdit*>(pAt)) {
                return true;
            }
            const QString className = QString::fromLatin1(pAt->metaObject()->className());
            if (className.contains(qsl("edbee"), Qt::CaseInsensitive) || className == qsl("TConsole") || className == qsl("TTextEdit")) {
                return true;
            }
        }
        return false;
    }

    // A view is read by its palette: a stylesheet only reaches a row through
    // ::item, and a ::item rule reaches the palette for some selectors only
    static QString inkRoleOf(const QWidget* pWidget, QColor& ink)
    {
        if (const auto* pView = qobject_cast<const QAbstractItemView*>(pWidget)) {
            ink = pView->palette().color(QPalette::Active, QPalette::Text);
            return qsl("its rows");
        }
        if (const auto* pGroup = qobject_cast<const QGroupBox*>(pWidget)) {
            if (pGroup->title().isEmpty() || (pGroup->isCheckable() && pGroup->isChecked())) {
                return QString();
            }
            ink = pGroup->palette().color(QPalette::Active, pGroup->foregroundRole());
            return qsl("its title");
        }
        if (const auto* pButton = qobject_cast<const QAbstractButton*>(pWidget)) {
            // Checked or pressed, a button is lit in the accent - a state, not an ink
            if (pButton->text().isEmpty() || pButton->isChecked() || pButton->isDown()) {
                return QString();
            }
            ink = pButton->palette().color(QPalette::Active, pButton->foregroundRole());
            return qsl("its text");
        }
        if (const auto* pLabel = qobject_cast<const QLabel*>(pWidget)) {
            if (pLabel->text().isEmpty()) {
                return QString();
            }
            ink = pLabel->palette().color(QPalette::Active, pLabel->foregroundRole());
            return qsl("its text");
        }
        return QString();
    }

    int walkTheChrome(const QString& appearance, const QColor& chrome, QStringList& misses) const
    {
        int audited = 0;
        for (const QWidget* pWidget : mpEditor->findChildren<QWidget*>()) {
            // Disabled words take disabledText and hovered ones the accent -
            // states, not inks
            if (!pWidget->isVisible() || pWidget->rect().isEmpty() || !pWidget->isEnabled() || pWidget->underMouse()) {
                continue;
            }
            if (insideAField(pWidget) || aStateChipOrAChipsOwnName(pWidget)) {
                continue;
            }

            QColor ink;
            const QString what = inkRoleOf(pWidget, ink);
            if (what.isEmpty() || !ink.isValid()) {
                continue;
            }
            ++audited;
            if (ink.rgb() == chrome.rgb()) {
                continue;
            }
            misses << qsl("%1 (%2), %3 appearance: %4 are %5, not the %6 every word of the editor's chrome is written in")
                              .arg(pWidget->objectName().isEmpty() ? qsl("<unnamed>") : pWidget->objectName(),
                                   QString::fromLatin1(pWidget->metaObject()->className()),
                                   appearance,
                                   what,
                                   ink.name(),
                                   chrome.name());
        }
        return audited;
    }

    void buildThreePatternRows()
    {
        mpEditor->showPatternItems(3);
        const QList<int> types{REGEX_PERL, REGEX_PROMPT, REGEX_COLOR_PATTERN};
        for (int row = 0; row < types.size(); ++row) {
            mpEditor->mTriggerPatternEdit.at(row)->comboBox_patternType->setCurrentIndex(types.at(row));
        }
        mpEditor->mTriggerPatternEdit.at(0)->singleLineTextEdit_pattern->setPlainText(qsl("^You (?:see|hear) (.+)$"));
        QCoreApplication::processEvents();
        QVERIFY2(mpEditor->mVisiblePatternCount == types.size(), qPrintable(qsl("the trigger form shows %1 pattern rows rather than three").arg(QString::number(mpEditor->mVisiblePatternCount))));
    }

    // An empty view shows a placeholder rather than its form; the events row gets
    // an own event and a sys* one so both of its inks are walked
    void buildAnItemInEveryOtherView()
    {
        mpEditor->slot_showAliases();
        mpEditor->addAlias(false);
        mpEditor->slot_showTimers();
        mpEditor->addTimer(false);
        mpEditor->slot_showKeys();
        mpEditor->addKey(false);
        mpEditor->slot_showActions();
        mpEditor->addAction(false);
        mpEditor->slot_showScripts();
        mpEditor->addScript(false);
        QTest::qWait(100ms);
        QVERIFY2(mpEditor->mpChipRow_scriptEvents != nullptr, "the scripts form has no events row for this walk to read");
        mpEditor->mpChipRow_scriptEvents->setItems({qsl("myOwnEvent"), qsl("sysLoadEvent")});
        QCoreApplication::processEvents();
    }

    void enterView(const EditorViewType view)
    {
        switch (view) {
        case EditorViewType::cmTriggerView:
            mpEditor->slot_showTriggers();
            break;
        case EditorViewType::cmAliasView:
            mpEditor->slot_showAliases();
            break;
        case EditorViewType::cmTimerView:
            mpEditor->slot_showTimers();
            break;
        case EditorViewType::cmScriptView:
            mpEditor->slot_showScripts();
            break;
        case EditorViewType::cmActionView:
            mpEditor->slot_showActions();
            break;
        case EditorViewType::cmKeysView:
            mpEditor->slot_showKeys();
            break;
        case EditorViewType::cmVarsView:
            mpEditor->slot_showVariables();
            break;
        case EditorViewType::cmUnknownView:
            break;
        }
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        mpEditor->grab();
    }

    // Through the settings control, the path that restyles these windows. The
    // grab() is there because painting settles a widget's palette against the
    // application's, and under the offscreen platform nothing paints unasked.
    void setAppearance(const enums::Appearance appearance)
    {
        if (mpPreferences->comboBox_appearance->currentIndex() == appearance) {
            mpPreferences->comboBox_appearance->setCurrentIndex(appearance == enums::Appearance::dark ? enums::Appearance::light : enums::Appearance::dark);
            QCoreApplication::processEvents();
        }
        mpPreferences->comboBox_appearance->setCurrentIndex(appearance);
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        QCoreApplication::processEvents();
        mpEditor->grab();
    }

    void writeFreshEditorThemesFile()
    {
        const QString file = MudletApp::getMudletPath(enums::editorWidgetThemeJsonFile);
        QVERIFY(QDir().mkpath(QFileInfo(file).absolutePath()));
        QFile themes(file);
        QVERIFY(themes.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QVERIFY(themes.write("[]") == 2);
    }

    // Off the whole window: grab() renders only the widget and its children, so a
    // transparent widget comes back as an unpainted pixmap
    QColor pixelOfWindowUnder(QWidget* pWidget, const QPoint& pointInWidget) const
    {
        const QPixmap shot = mpEditor->grab();
        return shot.toImage().pixelColor(pWidget->mapTo(mpEditor, pointInWidget));
    }


    static QString describe(const QColor& measured, const uiDesign::ThemeTokens& tokens)
    {
        return qsl("%1 - the page is %2, the pane %3, the card %4, the field %5, the separator %6")
                .arg(measured.name(), tokens.page.name(), tokens.pane.name(), tokens.card.name(), tokens.field.name(), tokens.separator.name());
    }

    // Under the last pattern row: the one place in the edit column that is
    // nothing but its surface
    QColor editColumnTone() const
    {
        auto* pPatternList = mpEditor->findChild<QWidget*>(qsl("editorPatternList"));
        if (!pPatternList || pPatternList->height() < 20 || pPatternList->width() < 20) {
            return QColor();
        }
        return pixelOfWindowUnder(pPatternList, QPoint(pPatternList->width() - 3, pPatternList->height() - 3));
    }

    // Inside the sidebar's bottom padding, clear of the pill a chosen row is
    // drawn as
    QColor sidebarTone() const
    {
        QWidget* pSidebar = mpEditor->findChild<QWidget*>(qsl("editorSidebarPane"));
        if (!pSidebar || pSidebar->height() < 40 || pSidebar->width() < 12) {
            return QColor();
        }
        return pixelOfWindowUnder(pSidebar, QPoint(4, pSidebar->height() - 5));
    }

    // Window and Base both white: themeTokens() then steps the page down under
    // the card, so a surface nobody paints, which falls back on Window, stops
    // matching the page tone by accident
    static QPalette theWhiteOnWhitePalette()
    {
        QPalette palette(QApplication::palette());
        palette.setColor(QPalette::Window, QColor(Qt::white));
        palette.setColor(QPalette::Base, QColor(Qt::white));
        palette.setColor(QPalette::WindowText, QColor(Qt::black));
        return palette;
    }

    // Setting the application palette sends the PaletteChange that
    // dlgTriggerEditor::changeEvent() restyles on, as an appearance change does
    void restyleWith(const QPalette& palette) const
    {
        QApplication::setPalette(palette);
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        QCoreApplication::processEvents();
        mpEditor->grab();
    }

    QColor itemPanelTone() const
    {
        QWidget* pTreeViewport = mpEditor->treeWidget_triggers->viewport();
        if (pTreeViewport->height() < 20 || pTreeViewport->width() < 20) {
            return QColor();
        }
        return pixelOfWindowUnder(pTreeViewport, QPoint(pTreeViewport->width() - 3, pTreeViewport->height() - 3));
    }

    // Mirrors the actions toolbar's padding in applyEditorShellStyle(), which the
    // grip sits inside
    static constexpr int scmToolBarPadding = 6;


    QSplitterHandle* paneSeam() const { return mpEditor->splitter_main->handle(1); }

    QSplitterHandle* codeHeading() const { return mpEditor->splitter_right->handle(1); }

    QList<QColor> acrossTheHandle(const QImage& shot, const QWidget* pHandle, const int y) const
    {
        QList<QColor> row;
        for (int x = 0; x < pHandle->width(); ++x) {
            row << shot.pixelColor(pHandle->mapTo(mpEditor, QPoint(x, y)));
        }
        return row;
    }

    static QString describe(const QList<QColor>& row)
    {
        QStringList names;
        for (const QColor& colour : row) {
            names << colour.name();
        }
        return names.join(QLatin1Char(' '));
    }

    static QPair<int, int> runOf(const QList<QColor>& row, const QColor& wanted)
    {
        int start = -1;
        int length = 0;
        for (int x = 0; x < row.size(); ++x) {
            if (row.at(x).rgb() != wanted.rgb()) {
                continue;
            }
            if (start < 0) {
                start = x;
            }
            ++length;
        }
        return {start, length};
    }

    void hover(QWidget* pWidget, const bool hovered) const
    {
        const QPointF local(pWidget->rect().center());
        if (hovered) {
            QEnterEvent enter(local, local, pWidget->mapToGlobal(pWidget->rect().center()));
            QCoreApplication::sendEvent(pWidget, &enter);
        } else {
            QEvent leave(QEvent::Leave);
            QCoreApplication::sendEvent(pWidget, &leave);
        }
        QTest::qWait(20ms);
    }

    // Generous: rounded edges and glyphs blend with what is behind them, and more
    // so on a screen that doubles its pixels
    static constexpr int scmMarkInkSlack = 40;

    QToolButton* saveProfileButton() const { return qobject_cast<QToolButton*>(mpEditor->toolBar->widgetForAction(mpEditor->mProfileSaveAction)); }

    QMenu* searchOptionsMenu() const { return mpEditor->findChild<QMenu*>(qsl("pMenu_searchOptions")); }

    // A menu's rules are carried by whatever it hangs off - the toolbar for the
    // button's menu, the menu itself for the one hanging off the window
    static QString sheetsReaching(const QWidget* pMenu)
    {
        QString sheets;
        for (const QObject* pAncestor = pMenu; pAncestor; pAncestor = pAncestor->parent()) {
            if (const auto* pWidget = qobject_cast<const QWidget*>(pAncestor)) {
                sheets += pWidget->styleSheet();
            }
        }
        return sheets;
    }

    // Popped up: a menu is polished against the sheets over it, and its window
    // made, only at its first show
    QImage grabOfMenu(QMenu* pMenu, const QPoint& where) const
    {
        pMenu->popup(where);
        QTest::qWaitForWindowExposed(pMenu);
        QCoreApplication::processEvents();
        QTest::qWait(50ms);
        const QImage shot = pMenu->grab().toImage().convertToFormat(QImage::Format_ARGB32);
        pMenu->hide();
        QCoreApplication::processEvents();
        return shot;
    }

    QPoint underTheSaveProfileButton() const
    {
        const QToolButton* pButton = saveProfileButton();
        return pButton ? pButton->mapToGlobal(QPoint(0, pButton->height())) : mpEditor->mapToGlobal(QPoint(20, 60));
    }

    // Asked of the style rather than clicked: a popup runs an event loop a case
    // cannot get back out of. Filled as QToolButton fills its own in
    // MenuButtonPopup mode.
    static QRect menuHalfOf(QToolButton* pButton)
    {
        QStyleOptionToolButton option;
        option.initFrom(pButton);
        option.rect = pButton->rect();
        option.text = pButton->text();
        option.icon = pButton->icon();
        option.iconSize = pButton->iconSize();
        option.toolButtonStyle = pButton->toolButtonStyle();
        option.features = QStyleOptionToolButton::Menu | QStyleOptionToolButton::MenuButtonPopup;
        option.subControls = QStyle::SC_ToolButton | QStyle::SC_ToolButtonMenu;
        return pButton->style()->subControlRect(QStyle::CC_ToolButton, &option, QStyle::SC_ToolButtonMenu, pButton);
    }


    // The wait lets the button take the hover state and be asked to paint
    static void pointAt(QWidget* pWidget, const QPoint& where)
    {
        QTest::mouseMove(pWidget, where);
        QCoreApplication::processEvents();
        QTest::qWait(30ms);
    }

    // A patch rather than a pixel: a wash is a few levels' difference, and one
    // pixel on a glyph or an antialiased edge would swamp it
    static QColor averageColour(const QImage& shot, const QRect& box)
    {
        const QRect inside = box.intersected(shot.rect());
        if (inside.isEmpty()) {
            return QColor();
        }
        qint64 red = 0;
        qint64 green = 0;
        qint64 blue = 0;
        for (int y = inside.top(); y <= inside.bottom(); ++y) {
            for (int x = inside.left(); x <= inside.right(); ++x) {
                const QColor sample = shot.pixelColor(x, y);
                red += sample.red();
                green += sample.green();
                blue += sample.blue();
            }
        }
        const qint64 pixels = static_cast<qint64>(inside.width()) * inside.height();
        return QColor(static_cast<int>(red / pixels), static_cast<int>(green / pixels), static_cast<int>(blue / pixels));
    }

    static QRect inTheGrabsPixels(const QImage& shot, const QRect& box)
    {
        const qreal ratio = shot.devicePixelRatio();
        return QRect(QPoint(qRound(box.left() * ratio), qRound(box.top() * ratio)), QPoint(qRound((box.right() + 1) * ratio) - 1, qRound((box.bottom() + 1) * ratio) - 1)).intersected(shot.rect());
    }

    static int markedPixelsIn(const QImage& shot, const QRect& box, const QColor& fill)
    {
        const QRect inside = box.adjusted(box.width() / 4, box.height() / 4, -box.width() / 4, -box.height() / 4);
        int marked = 0;
        for (int y = inside.top(); y <= inside.bottom(); ++y) {
            for (int x = inside.left(); x <= inside.right(); ++x) {
                if (TestPixels::distance(shot.pixelColor(x, y), fill) > scmMarkInkSlack) {
                    ++marked;
                }
            }
        }
        return marked;
    }

    // Found by walking in from the row's edge rather than named, since where the
    // mark stands in the row's leading room is the recipe's own business
    static QRect markBoxOf(const QMenu* pMenu, QAction* pAction, const QImage& shot)
    {
        const QRect row = inTheGrabsPixels(shot, pMenu->actionGeometry(pAction));
        const int middle = row.center().y();
        const QColor surface = shot.pixelColor(row.left(), middle);
        const int size = qRound(uiDesign::scmChoiceIndicatorSize * shot.devicePixelRatio());
        for (int x = row.left(); x < std::min(row.left() + 3 * size, row.right()); ++x) {
            if (TestPixels::distance(shot.pixelColor(x, middle), surface) > scmMarkInkSlack) {
                return QRect(x, middle - size / 2, size, size).intersected(shot.rect());
            }
        }
        return QRect();
    }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.startMudlet(), qPrintable(mSession.why()));
        writeFreshEditorThemesFile();
        QVERIFY2(mSession.openProfile(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();

        mudlet::self()->slot_showTriggerDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
        mpEditor->resize(1200, 800);
        buildAnItemInEveryOtherView();
        if (QTest::currentTestFailed()) {
            return;
        }
        // The heading over the code pane is only laid out once the form above it
        // holds an item
        mpEditor->slot_showTriggers();
        mpEditor->addTrigger(false);
        QTest::qWait(100ms);
        buildThreePatternRows();
        QTest::qWait(100ms);
        QVERIFY(QTest::qWaitForWindowExposed(mpEditor));

        QVERIFY2(paneSeam() != nullptr, "The main splitter has no handle between its two panes");
        QVERIFY2(codeHeading() != nullptr, "The right hand splitter has no handle over the code pane");

        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        mpPreferences->setStyleSheet(mpHost->mProfileStyleSheet);
        mpPreferences->resize(1060, 760);
        mpPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpPreferences));
        mAppearanceAtStart = mudlet::self()->mAppearance;
    }

    void cleanupTestCase()
    {
        delete mpPreferences;
        mpPreferences = nullptr;
        mpEditor = nullptr;
        mpHost = nullptr;
        if (mudlet::self()) {
            mudlet::self()->setAppearance(enums::Appearance::systemSetting);
        }
        mSession.stop();
    }

    // One list at the end: stopping at the first miss would take a run per miss
    // to clear
    void test_everyWordOutsideAFieldIsTheOneQuietTone()
    {
        // Put back on the way out: the cases after this one read the window under
        // the appearance the run opened in
        const auto putBack = qScopeGuard([this]() {
            setAppearance(mAppearanceAtStart);
        });
        QStringList misses;
        QStringList counts;
        const QList<EditorViewType> views{EditorViewType::cmTriggerView,
                                          EditorViewType::cmAliasView,
                                          EditorViewType::cmTimerView,
                                          EditorViewType::cmScriptView,
                                          EditorViewType::cmActionView,
                                          EditorViewType::cmKeysView,
                                          EditorViewType::cmVarsView};
        for (const auto& appearance : QList<QPair<QString, enums::Appearance>>{{qsl("dark"), enums::Appearance::dark}, {qsl("light"), enums::Appearance::light}}) {
            setAppearance(appearance.second);
            const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
            int audited = 0;
            for (const EditorViewType view : views) {
                enterView(view);
                audited += walkTheChrome(appearance.first, tokens.mutedText, misses);
            }
            counts << qsl("%1: %2 things in %3").arg(appearance.first, QString::number(audited), tokens.mutedText.name());
            QVERIFY2(audited >= scmLeastAudited,
                     qPrintable(qsl("only %1 things were read on the %2 appearance, against the %3 this walk reaches - it stopped finding widgets rather than finding them all in one ink")
                                        .arg(QString::number(audited), appearance.first, QString::number(scmLeastAudited))));
        }

        qInfo().noquote() << qsl("  audited %1").arg(counts.join(qsl("; ")));
        for (const QString& miss : misses) {
            qWarning().noquote() << miss;
        }
        QVERIFY2(misses.isEmpty(), qPrintable(qsl("%1 thing(s) in the editor's chrome are written in something other than the one quiet tone - listed above").arg(QString::number(misses.size()))));
    }

    // A palette a view never draws from would pass the walk above while the tree
    // paints in the platform's white
    void test_theTreesHeadingRowIsPaintedInThatSameTone()
    {
        // Put back on the way out: the cases after this one read the window under
        // the appearance the run opened in
        const auto putBack = qScopeGuard([this]() {
            setAppearance(mAppearanceAtStart);
        });
        setAppearance(enums::Appearance::dark);
        // The panel shows one tree at a time, and the walk above leaves the last
        // view up
        enterView(EditorViewType::cmTriggerView);
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();

        QTreeWidget* pTree = mpEditor->treeWidget_triggers;
        QTreeWidgetItem* pHeading = mpEditor->mpTriggerBaseItem;
        QVERIFY2(pTree && pHeading, "the triggers tree has no heading row to read");
        QVERIFY2(!pHeading->isSelected(), "the heading row is chosen, so what it is painted in is the accent rather than the chrome tone");

        const QRect row = pTree->visualItemRect(pHeading);
        QVERIFY2(row.width() > 20 && row.height() > 4, "the heading row is too small to read a pixel out of");

        const QImage shot = mpEditor->grab().toImage();
        // grab() answers a pixmap at the screen's device pixel ratio, so the
        // window's own coordinates are not the image's on a scaled display
        const qreal ratio = shot.width() / static_cast<qreal>(std::max(1, mpEditor->width()));
        const QPoint topLeft = pTree->viewport()->mapTo(mpEditor, row.topLeft());
        const QRect painted = QRect(QPoint(qRound(topLeft.x() * ratio), qRound(topLeft.y() * ratio)), QSize(qRound(row.width() * ratio), qRound(row.height() * ratio))).intersected(shot.rect());
        QVERIFY2(painted.width() > 8 && painted.height() > 4, "the heading row does not land on the shot of the window");

        const auto distanceFromThePane = [&tokens](const QColor& colour) {
            return TestPixels::distance(colour, tokens.pane);
        };
        QColor ink = tokens.pane;
        for (int y = painted.top(); y <= painted.bottom(); ++y) {
            for (int x = painted.left(); x <= painted.right(); ++x) {
                const QColor pixel = shot.pixelColor(x, y);
                if (distanceFromThePane(pixel) > distanceFromThePane(ink)) {
                    ink = pixel;
                }
            }
        }

        qInfo().noquote()
                << qsl("  the Triggers heading is painted %1, against the %2 the editor's chrome is written in, on the %3 the panel is").arg(ink.name(), tokens.mutedText.name(), tokens.pane.name());
        const int off = std::max({std::abs(ink.red() - tokens.mutedText.red()), std::abs(ink.green() - tokens.mutedText.green()), std::abs(ink.blue() - tokens.mutedText.blue())});
        QVERIFY2(off <= scmInkTolerance,
                 qPrintable(qsl("the Triggers heading is painted %1, %2 off the %3 every word of the editor's chrome is written in").arg(ink.name(), QString::number(off), tokens.mutedText.name())));
    }

    void test_theFormBehindThePatternsIsThePageTone()
    {
        auto* pPatternList = mpEditor->findChild<QWidget*>(qsl("editorPatternList"));
        QVERIFY2(pPatternList, "the widget the pattern rows are held in has gone, or been renamed");
        QVERIFY2(pPatternList->height() > 20 && pPatternList->width() > 20, "the pattern list is too small to read a pixel out of");

        const QPalette savedPalette = QApplication::palette();
        const auto putBack = qScopeGuard([this, savedPalette] {
            restyleWith(savedPalette);
        });
        restyleWith(theWhiteOnWhitePalette());

        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        QVERIFY2(tokens.page.rgb() != QApplication::palette().color(QPalette::Window).rgb(),
                 qPrintable(qsl("the page is %1 and QPalette::Window %2 - they agree, so a surface nothing painted would read as the page and this case would measure nothing")
                                    .arg(tokens.page.name(), QApplication::palette().color(QPalette::Window).name())));

        const QColor measured = pixelOfWindowUnder(pPatternList, QPoint(pPatternList->width() - 3, pPatternList->height() - 3));
        qInfo().noquote() << qsl("  the form behind the patterns is painted %1").arg(describe(measured, tokens));

        QVERIFY2(TestPixels::distance(measured, tokens.page) < TestPixels::distance(measured, tokens.field),
                 qPrintable(qsl("the form behind the patterns is painted %1 - it is nearer the field colour than the page, which is the sunken box the edit column used to read as")
                                    .arg(describe(measured, tokens))));
        QCOMPARE(measured.rgb(), tokens.page.rgb());
    }

    void test_theSidebarAndTheEditColumnAreBothThePage()
    {
        const QPalette savedPalette = QApplication::palette();
        const auto putBack = qScopeGuard([this, savedPalette] {
            restyleWith(savedPalette);
        });
        restyleWith(theWhiteOnWhitePalette());

        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QColor windowColour = QApplication::palette().color(QPalette::Window);
        QVERIFY2(tokens.page.rgb() != windowColour.rgb(),
                 qPrintable(qsl("the page is %1 and QPalette::Window %2 - they agree, so a column nothing painted would read as the page").arg(tokens.page.name(), windowColour.name())));

        const QColor sidebar = sidebarTone();
        const QColor form = editColumnTone();
        QVERIFY2(sidebar.isValid() && form.isValid(), "the sidebar or the edit column is too small to read a pixel out of");

        // A control the editor paints nothing on. Its palette is set rather than
        // inherited: a stylesheet's background-color lands on the polished shell's
        // QPalette::Window, so an inheriting child would fill with the page tone.
        QWidget* pShell = mpEditor->findChild<QWidget*>(qsl("editorShell"));
        QVERIFY2(pShell, "the editor shell has gone, or been renamed");
        QWidget unpaintedProbe(pShell);
        unpaintedProbe.setPalette(QApplication::palette());
        unpaintedProbe.setAutoFillBackground(true);
        unpaintedProbe.setGeometry(pShell->width() / 2 - 20, pShell->height() / 2 - 20, 40, 40);
        unpaintedProbe.show();
        QCoreApplication::processEvents();
        const QColor unpainted = pixelOfWindowUnder(&unpaintedProbe, QPoint(unpaintedProbe.width() / 2, unpaintedProbe.height() / 2));

        qInfo().noquote() << qsl("measured: sidebar %1, edit column %2, panel %3, an unpainted widget %4; tokens: page %5, pane %6, separator %7, card %8")
                                     .arg(sidebar.name(), form.name(), itemPanelTone().name(), unpainted.name(), tokens.page.name(), tokens.pane.name(), tokens.separator.name(), tokens.card.name());

        QVERIFY2(sidebar.rgb() == form.rgb(), qPrintable(qsl("the sidebar is painted %1 and the column an item is edited in %2").arg(sidebar.name(), form.name())));
        QVERIFY2(sidebar.rgb() == tokens.page.rgb(), qPrintable(qsl("the sidebar is painted %1").arg(describe(sidebar, tokens))));
        QVERIFY2(form.rgb() == tokens.page.rgb(), qPrintable(qsl("the column an item is edited in is painted %1").arg(describe(form, tokens))));
        QVERIFY2(
                unpainted.rgb() != tokens.page.rgb(),
                qPrintable(qsl("a widget nothing paints reads %1, which is the page tone - so the two readings above would hold for a column nobody painted either").arg(describe(unpainted, tokens))));
    }

    void test_theItemPanelIsAPaneLiftedOffThePage()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QColor panel = itemPanelTone();
        const QColor form = editColumnTone();
        QVERIFY2(panel.isValid() && form.isValid(), "the item panel or the edit column is too small to read a pixel out of");

        QVERIFY2(panel.rgb() != form.rgb(), qPrintable(qsl("the panel of items and the column an item is edited in are both painted %1, so the window is one flat expanse").arg(panel.name())));
        QVERIFY2(panel.rgb() == tokens.pane.rgb(), qPrintable(qsl("the panel of items is painted %1").arg(describe(panel, tokens))));
        QVERIFY2(panel.lightness() > tokens.page.lightness(), qPrintable(qsl("the panel of items is not lifted off the page: it is painted %1").arg(describe(panel, tokens))));
        QVERIFY2(TestPixels::distance(panel, tokens.page) < TestPixels::distance(panel, tokens.card),
                 qPrintable(qsl("the panel of items is nearer a card than the page it lies beside: it is painted %1").arg(describe(panel, tokens))));
    }

    void test_theSplitterHandlesAreSeparatorsDarkerThanThePage()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();

        QSplitterHandle* pPanelHandle = mpEditor->splitter_main->handle(1);
        QVERIFY2(pPanelHandle && pPanelHandle->width() > 4 && pPanelHandle->height() > 80, "the handle between the panel and the edit column is missing or too small to read a pixel out of");
        // Clear of the middle of its length, where a splitter may draw a grip
        const QColor panelSeam = pixelOfWindowUnder(pPanelHandle, QPoint(pPanelHandle->width() / 2, 6));

        QSplitterHandle* pCodeHandle = mpEditor->splitter_right->handle(1);
        QVERIFY2(pCodeHandle && pCodeHandle->height() > 8 && pCodeHandle->width() > 80, "the handle carrying the code pane's heading is missing or too small to read a pixel out of");
        // Above the heading and the compile chip, both of which are drawn down
        // the middle of the strip
        const QColor codeSeam = pixelOfWindowUnder(pCodeHandle, QPoint(pCodeHandle->width() / 4, 1));

        QVERIFY2(codeSeam.rgb() == tokens.separator.rgb(), qPrintable(qsl("the handle carrying the code pane's heading is painted %1").arg(describe(codeSeam, tokens))));
        QVERIFY2(panelSeam.lightness() < tokens.page.lightness(), qPrintable(qsl("the seam between two panes is not darker than the page: it is painted %1").arg(describe(panelSeam, tokens))));
    }

    void test_theNameFieldKeepsTheFieldTone()
    {
        QLineEdit* pName = mpEditor->mpTriggersMainArea->lineEdit_trigger_name;
        QVERIFY2(pName->height() > 8 && pName->width() > 40, "the trigger name field is too small to read a pixel out of");

        const QPalette savedPalette = QApplication::palette();
        const auto putBack = qScopeGuard([this, savedPalette] {
            restyleWith(savedPalette);
        });
        restyleWith(theWhiteOnWhitePalette());

        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        QVERIFY2(tokens.field.rgb() != tokens.page.rgb(),
                 qPrintable(qsl("the field surface and the page are both %1 on this palette, so a field painted as neither would still pass").arg(tokens.field.name())));

        // Near the trailing edge: the name is drawn from the leading edge, so the
        // middle of the field is a letter once the name is long enough
        const QColor measured = pixelOfWindowUnder(pName, QPoint(pName->width() - 6, pName->height() / 2));
        qInfo().noquote() << qsl("  the trigger name field is painted %1").arg(describe(measured, tokens));

        QVERIFY2(measured.rgb() == tokens.field.rgb(), qPrintable(qsl("the trigger name field is painted %1 - it has lost the sunken colour a field is filled with").arg(describe(measured, tokens))));
    }

    // Scoped by id so the rule paints the shell alone and lets everything under
    // it show through, rather than painting over what draws it
    void test_theShellRuleNamesNothingButTheShell()
    {
        QWidget* pShell = mpEditor->findChild<QWidget*>(qsl("editorShell"));
        QVERIFY2(pShell, "the editor shell has gone, or been renamed");
        const QString sheet = pShell->styleSheet();

        QVERIFY2(sheet.contains(qsl("#editorShell {")), qPrintable(qsl("the shell is not painted at all, its stylesheet is \"%1\"").arg(sheet)));
        QVERIFY2(sheet.count(QLatin1Char('{')) == 1, qPrintable(qsl("the shell's stylesheet carries more than the one rule that paints it: \"%1\"").arg(sheet)));
        QVERIFY2(sheet.contains(uiDesign::themeTokens().page.name()), qPrintable(qsl("the shell is painted something other than the page tone: \"%1\"").arg(sheet)));
    }

    // Every muted tone is mixed from the softened text colour, so the whole scale
    // loses contrast with it - measured here rather than assumed
    void test_theWordsAreSoftenedAndADarkFieldIsLifted()
    {
        const QPalette savedPalette = QApplication::palette();

        QPalette darkPalette(savedPalette);
        darkPalette.setColor(QPalette::Window, QColor(0x35, 0x35, 0x35));
        darkPalette.setColor(QPalette::WindowText, QColor(Qt::white));
        darkPalette.setColor(QPalette::Base, QColor(0x19, 0x19, 0x19));

        QPalette lightPalette(savedPalette);
        lightPalette.setColor(QPalette::Window, QColor(0xec, 0xec, 0xec));
        lightPalette.setColor(QPalette::WindowText, QColor(Qt::black));
        lightPalette.setColor(QPalette::Base, QColor(Qt::white));

        for (const QPalette& palette : {darkPalette, lightPalette}) {
            QApplication::setPalette(palette);
            const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
            const QString theme = tokens.darkPage ? qsl("dark") : qsl("light");
            const QColor paletteText = palette.color(QPalette::WindowText);
            const QColor paletteField = palette.color(QPalette::Base);

            const qreal onPage = uiDesign::contrastRatio(tokens.text, tokens.page);
            const qreal onCard = uiDesign::contrastRatio(tokens.text, tokens.card);
            const qreal onField = uiDesign::contrastRatio(tokens.text, tokens.field);
            const qreal mutedOnPage = uiDesign::contrastRatio(tokens.mutedText, tokens.page);
            const qreal disabledOnPage = uiDesign::contrastRatio(tokens.disabledText, tokens.page);
            const qreal disabledOnField = uiDesign::contrastRatio(tokens.disabledText, tokens.field);
            qInfo().noquote()
                    << qsl("%1: text %2 (palette %3), field %4 (palette %5), page %6, card %7, muted %8 - text reads %9:1 on the page, %10:1 on a card, %11:1 in a field; muted %12:1 on the page")
                               .arg(theme,
                                    tokens.text.name(),
                                    paletteText.name(),
                                    tokens.field.name(),
                                    paletteField.name(),
                                    tokens.page.name(),
                                    tokens.card.name(),
                                    tokens.mutedText.name(),
                                    QString::number(onPage, 'f', 2),
                                    QString::number(onCard, 'f', 2),
                                    QString::number(onField, 'f', 2),
                                    QString::number(mutedOnPage, 'f', 2));
            qInfo().noquote() << qsl("%1: an unavailable word is %2, reading %3:1 on the page and %4:1 in a field")
                                         .arg(theme, tokens.disabledText.name(), QString::number(disabledOnPage, 'f', 2), QString::number(disabledOnField, 'f', 2));

            QVERIFY2(tokens.text.rgb() != paletteText.rgb(),
                     qPrintable(qsl("on the %1 theme the words are the palette's own %2, which is the glare the softening is for").arg(theme, paletteText.name())));
            const int towardsThePage = std::abs(tokens.text.lightness() - paletteText.lightness());
            const int roomToThePage = std::abs(tokens.page.lightness() - paletteText.lightness());
            QVERIFY2(towardsThePage > 0 && towardsThePage < roomToThePage / 2,
                     qPrintable(qsl("on the %1 theme the words are %2, %3 levels off the palette's %4 with only %5 between that and the page - that is a wash, not a softening")
                                        .arg(theme, tokens.text.name(), QString::number(towardsThePage), paletteText.name(), QString::number(roomToThePage))));

            for (const auto& reading : {QPair<QString, qreal>{qsl("the page"), onPage}, {qsl("a card"), onCard}, {qsl("a field"), onField}}) {
                QVERIFY2(reading.second >= 4.5,
                         qPrintable(qsl("on the %1 theme the words read %2:1 against %3, under the 4.5:1 body text has to clear").arg(theme, QString::number(reading.second, 'f', 2), reading.first)));
            }
            QVERIFY2(mutedOnPage >= 3.0,
                     qPrintable(qsl("on the %1 theme the quiet tone reads %2:1 on the page, under the 3:1 a large or secondary line has to clear").arg(theme, QString::number(mutedOnPage, 'f', 2))));

            // A third of the way from page to text came to about 2.3:1 on a light
            // theme, leaving the disabled prompt-line pattern row all but invisible
            for (const auto& reading : {QPair<QString, qreal>{qsl("the page"), disabledOnPage}, {qsl("a field"), disabledOnField}}) {
                QVERIFY2(reading.second >= 3.0,
                         qPrintable(qsl("on the %1 theme an unavailable word is %2, reading %3:1 against %4 - under the 3:1 a secondary line has to clear, and a word nobody can read is not a word "
                                        "switched off")
                                            .arg(theme, tokens.disabledText.name(), QString::number(reading.second, 'f', 2), reading.first)));
            }
            QVERIFY2(disabledOnPage < mutedOnPage,
                     qPrintable(qsl("on the %1 theme an unavailable word reads %2:1 on the page against the quiet tone's %3:1, so the two say the same thing")
                                        .arg(theme, QString::number(disabledOnPage, 'f', 2), QString::number(mutedOnPage, 'f', 2))));

            if (!tokens.darkPage) {
                // Lifting a light theme's white Base towards a grey page only muddies it
                QCOMPARE(tokens.field.rgb(), paletteField.rgb());
                continue;
            }
            QVERIFY2(tokens.field.lightness() > paletteField.lightness() && tokens.field.lightness() < tokens.page.lightness(),
                     qPrintable(qsl("on the dark theme the field is %1, which is not between the palette's %2 and the page's %3 - a well is sunk into the page, not cut through it")
                                        .arg(tokens.field.name(), paletteField.name(), tokens.page.name())));
        }

        QApplication::setPalette(savedPalette);
        QTest::qWait(50ms);
    }

    // The tones are mixed from the palette, so both themes are put to the tokens
    // rather than trusting whichever one the run is under. Put back afterwards,
    // as the window restyles off the application's palette.
    void test_thePaneAndTheSeamHoldOnBothThemes()
    {
        const QPalette savedPalette = QApplication::palette();

        QPalette darkPalette(savedPalette);
        darkPalette.setColor(QPalette::Window, QColor(0x2c, 0x2c, 0x2e));
        darkPalette.setColor(QPalette::WindowText, QColor(Qt::white));
        darkPalette.setColor(QPalette::Base, QColor(0x1e, 0x1e, 0x1e));

        QPalette lightPalette(savedPalette);
        lightPalette.setColor(QPalette::Window, QColor(0xec, 0xec, 0xec));
        lightPalette.setColor(QPalette::WindowText, QColor(Qt::black));
        lightPalette.setColor(QPalette::Base, QColor(Qt::white));

        for (const QPalette& palette : {darkPalette, lightPalette}) {
            QApplication::setPalette(palette);
            const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
            const QString theme = tokens.darkPage ? qsl("dark") : qsl("light");
            qInfo().noquote() << qsl("%1: page %2, pane %3, separator %4, card %5, border %6")
                                         .arg(theme, tokens.page.name(), tokens.pane.name(), tokens.separator.name(), tokens.card.name(), tokens.border.name());

            const int paneLift = tokens.pane.lightness() - tokens.page.lightness();
            const int cardLift = tokens.card.lightness() - tokens.page.lightness();
            const int seamDrop = tokens.page.lightness() - tokens.separator.lightness();

            QVERIFY2(paneLift > 0, qPrintable(qsl("on the %1 theme the pane is not lifted off the page at all: %2 against %3").arg(theme, tokens.pane.name(), tokens.page.name())));
            QVERIFY2(paneLift * 3 < cardLift,
                     qPrintable(qsl("on the %1 theme the pane is %2 levels off the page and a card is %3 - the pane reads as a panel laid on top").arg(theme).arg(paneLift).arg(cardLift)));
            QVERIFY2(seamDrop >= 4, qPrintable(qsl("on the %1 theme the seam is only %2 levels under the page, which is no groove at all").arg(theme).arg(seamDrop)));
            // A fraction of the room under the page rather than a fixed number of
            // levels, so a light page gets a groove rather than a black rule
            QVERIFY2(seamDrop < tokens.page.lightness() / 2,
                     qPrintable(qsl("on the %1 theme the seam is %2 levels under a page of %3, which reads as a rule drawn across the window").arg(theme).arg(seamDrop).arg(tokens.page.lightness())));
        }

        QApplication::setPalette(savedPalette);
        QTest::qWait(50ms);
    }

    void test_aPlainHandleIsOneHairlineBetweenTwoTones()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        QSplitterHandle* pHandle = paneSeam();
        const QList<QColor> row = acrossTheHandle(TestPixels::shot(mpEditor), pHandle, pHandle->height() / 2);
        const QPair<int, int> seam = runOf(row, tokens.separator);

        qInfo().noquote() << qsl("  across the %1px handle: %2; the seam is %3, the panel %4 and the page %5")
                                     .arg(QString::number(pHandle->width()), describe(row), tokens.separator.name(), tokens.pane.name(), tokens.page.name());

        QCOMPARE(pHandle->width(), uiDesign::GripSplitter::scmHandleThickness);
        QVERIFY2(seam.second == 1, qPrintable(qsl("the seam is %1px of %2 rather than one: %3").arg(QString::number(seam.second), tokens.separator.name(), describe(row))));

        QStringList wrong;
        for (int x = 0; x < row.size(); ++x) {
            if (x == seam.first) {
                continue;
            }
            const QColor wanted = x < seam.first ? tokens.pane : tokens.page;
            if (row.at(x).rgb() != wanted.rgb()) {
                wrong << qsl("x=%1 is %2 where the pane beside it is %3").arg(QString::number(x), row.at(x).name(), wanted.name());
            }
        }
        QVERIFY2(wrong.isEmpty(), qPrintable(qsl("the handle is not the two panes' own tones either side of the seam: %1").arg(wrong.join(qsl(", ")))));
    }

    void test_hoveringAPlainHandleWidensTheSeamToTheAccent()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        QSplitterHandle* pHandle = paneSeam();
        hover(pHandle, true);
        const QImage shot = TestPixels::shot(mpEditor);
        // Three heights rather than one: a three-pixel grip pill drawn across the
        // middle of the handle would pass a single reading taken there
        const QList<QPair<QString, QList<QColor>>> rows{{qsl("near its top"), acrossTheHandle(shot, pHandle, 4)},
                                                        {qsl("across its middle"), acrossTheHandle(shot, pHandle, pHandle->height() / 2)},
                                                        {qsl("near its foot"), acrossTheHandle(shot, pHandle, pHandle->height() - 5)}};
        hover(pHandle, false);

        const int middle = (pHandle->width() - 1) / 2;
        QStringList measured;
        QStringList wrong;
        for (const auto& row : rows) {
            const QPair<int, int> lit = runOf(row.second, tokens.accent);
            measured << qsl("%1 %2").arg(row.first, describe(row.second));
            if (lit.second != 3) {
                wrong << qsl("%1 the seam is %2px of the accent rather than three").arg(row.first, QString::number(lit.second));
            } else if (lit.first != middle - 1) {
                wrong << qsl("%1 the seam starts at x=%2 rather than one either side of the middle at x=%3").arg(row.first, QString::number(lit.first), QString::number(middle));
            }
        }
        qInfo().noquote() << qsl("  hovered: %1; the accent is %2").arg(measured.join(qsl("; ")), tokens.accent.name());
        QVERIFY2(wrong.isEmpty(), qPrintable(qsl("the hovered handle is not one lit seam from end to end: %1").arg(wrong.join(qsl(", ")))));

        const QList<QColor> atRest = acrossTheHandle(TestPixels::shot(mpEditor), pHandle, pHandle->height() / 2);
        QVERIFY2(runOf(atRest, tokens.accent).second == 0, qPrintable(qsl("the seam stayed lit after the pointer left it: %1").arg(describe(atRest))));
    }

    void test_theCodePanesHeadingIsCutToThePanelsCorner()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        QSplitterHandle* pHandle = codeHeading();
        const QImage shot = TestPixels::shot(mpEditor);
        const QList<QColor> topRow = acrossTheHandle(shot, pHandle, 0);
        const QList<QColor> bottomRow = acrossTheHandle(shot, pHandle, pHandle->height() - 1);

        const QColor topCorner = topRow.constFirst();
        const QColor topMiddle = topRow.at(topRow.size() / 2);
        const QColor bottomCorner = bottomRow.constFirst();
        qInfo().noquote() << qsl("  the heading is %1px tall; its top corner is %2, the middle of its top edge %3 and its bottom corner %4; the strip is %5 and the page %6")
                                     .arg(QString::number(pHandle->height()), topCorner.name(), topMiddle.name(), bottomCorner.name(), tokens.separator.name(), tokens.page.name());

        QVERIFY2(topMiddle.rgb() == tokens.separator.rgb(), qPrintable(qsl("the heading strip does not reach the top of the handle: the middle of that edge is %1").arg(topMiddle.name())));
        QVERIFY2(topCorner.rgb() == tokens.page.rgb(), qPrintable(qsl("the heading's top corner is not cut away to the page: it is %1").arg(topCorner.name())));
        QVERIFY2(bottomCorner.rgb() == tokens.separator.rgb(), qPrintable(qsl("the heading's bottom corner is cut away as well, at %1 - only its top meets the page").arg(bottomCorner.name())));
    }

    void test_theHeadingCarriesNoGrip()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        QSplitterHandle* pHandle = codeHeading();
        const QColor middle = TestPixels::shot(mpEditor).pixelColor(pHandle->mapTo(mpEditor, pHandle->rect().center()));
        qInfo().noquote()
                << qsl("  the middle of the %1x%2 strip is %3; the strip is %4").arg(QString::number(pHandle->width()), QString::number(pHandle->height()), middle.name(), tokens.separator.name());
        QVERIFY2(middle.rgb() == tokens.separator.rgb(),
                 qPrintable(qsl("something is drawn across the middle of the heading: it is %1 rather than the strip's own %2").arg(middle.name(), tokens.separator.name())));
    }

    void test_hoveringTheHeadingLightsItsTopEdge()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        QSplitterHandle* pHandle = codeHeading();
        // The case above may leave the handle hovered, and entering a handle that
        // is already hovered lights nothing new
        hover(pHandle, false);

        const int x = pHandle->width() / 2;
        const auto toneAt = [&](const QImage& shot, const int y) {
            return shot.pixelColor(pHandle->mapTo(mpEditor, QPoint(x, y)));
        };

        hover(pHandle, true);
        const QImage lit = TestPixels::shot(mpEditor);
        QList<QColor> band;
        for (int y = 0; y < 4; ++y) {
            band << toneAt(lit, y);
        }
        hover(pHandle, false);
        const QColor atRest = toneAt(TestPixels::shot(mpEditor), 0);

        qInfo().noquote() << qsl("  hovered, the first four rows are %1; at rest the top row is %2. The accent is %3 and the strip %4")
                                     .arg(describe(band), atRest.name(), tokens.accent.name(), tokens.separator.name());

        QStringList wrong;
        for (int y = 0; y < 3; ++y) {
            if (band.at(y).rgb() != tokens.accent.rgb()) {
                wrong << qsl("row %1 is %2").arg(QString::number(y), band.at(y).name());
            }
        }
        QVERIFY2(wrong.isEmpty(), qPrintable(qsl("the hovered heading is not three pixels of the accent along its top edge: %1").arg(wrong.join(qsl(", ")))));
        QVERIFY2(band.at(3).rgb() == tokens.separator.rgb(),
                 qPrintable(qsl("the lit band is deeper than three pixels: the fourth row is %1 rather than the strip's own %2").arg(band.at(3).name(), tokens.separator.name())));
        QVERIFY2(atRest.rgb() == tokens.separator.rgb(), qPrintable(qsl("the top edge stayed lit after the pointer left it: it is %1").arg(atRest.name())));
    }

    // Five of the seven forms are fixed fields whose heading resizes nothing -
    // see EditorFormShellTest
    void test_aHeadingThatDragsNothingStaysDark()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        QSplitterHandle* pHandle = codeHeading();
        hover(pHandle, false);

        const int x = pHandle->width() / 2;
        const auto topRow = [&](const QImage& shot) {
            return shot.pixelColor(pHandle->mapTo(mpEditor, QPoint(x, 0)));
        };

        QVERIFY2(mpEditor->splitter_right->setHandleResizes(1, false), "the code pane's heading is not a handle this can be asked of");
        hover(pHandle, true);
        const QColor whileInert = topRow(TestPixels::shot(mpEditor));
        hover(pHandle, false);

        // Control: a band that never appears would otherwise pass this case
        QVERIFY(mpEditor->splitter_right->setHandleResizes(1, true));
        hover(pHandle, true);
        const QColor whileDragging = topRow(TestPixels::shot(mpEditor));
        hover(pHandle, false);

        qInfo().noquote() << qsl("  hovered, the top row reads %1 while the heading drags nothing and %2 while it drags; the strip is %3 and the accent %4")
                                     .arg(whileInert.name(), whileDragging.name(), tokens.separator.name(), tokens.accent.name());

        QVERIFY2(whileInert.rgb() == tokens.separator.rgb(),
                 qPrintable(qsl("a heading that resizes nothing lit its top edge anyway: it is %1 rather than the strip's own %2").arg(whileInert.name(), tokens.separator.name())));
        QVERIFY2(whileDragging.rgb() == tokens.accent.rgb(),
                 qPrintable(qsl("the heading did not light when it does drag, so the case above proves nothing: the top row is %1").arg(whileDragging.name())));
    }

    // Styling the bar drops the platform's handle; the fallback draws two dots a
    // shade off the bar
    void test_theToolbarsGripIsTheSixDotsInTheQuietTone()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        QToolBar* pToolBar = mpEditor->toolBar;
        QVERIFY2(pToolBar != nullptr && pToolBar->isMovable(), "The actions toolbar is missing or cannot be moved, so it carries no grip");
        const int extent = pToolBar->style()->pixelMetric(QStyle::PM_ToolBarHandleExtent, nullptr, pToolBar);
        const int strip = extent + scmToolBarPadding;

        const QImage shot = TestPixels::shot(mpEditor);
        int inked = 0;
        qreal strongest = 0.0;
        QColor strongestColour;
        QRect dots;
        for (int x = 0; x < strip; ++x) {
            for (int y = 0; y < pToolBar->height() - 1; ++y) {
                const QColor measured = shot.pixelColor(pToolBar->mapTo(mpEditor, QPoint(x, y)));
                // A fraction rather than a match, as two-pixel dots are
                // antialiased; qAbs, as on a dark page the ink is lighter than
                // the bar
                const qreal towardsTheInk = static_cast<qreal>(qAbs(measured.green() - tokens.page.green())) / std::max(1, qAbs(tokens.mutedText.green() - tokens.page.green()));
                if (towardsTheInk < 0.1) {
                    continue;
                }
                ++inked;
                dots = dots.isNull() ? QRect(x, y, 1, 1) : dots.united(QRect(x, y, 1, 1));
                if (towardsTheInk > strongest) {
                    strongest = towardsTheInk;
                    strongestColour = measured;
                }
            }
        }
        qInfo().noquote() << qsl("  the handle is %1px wide; the grip is a %2x%3 block of %4 painted pixels at %5,%6, its strongest %7 - %8 of the way from the bar at %9 to the quiet tone at %10")
                                     .arg(QString::number(extent),
                                          QString::number(dots.width()),
                                          QString::number(dots.height()),
                                          QString::number(inked),
                                          QString::number(dots.x()),
                                          QString::number(dots.y()),
                                          strongestColour.isValid() ? strongestColour.name() : qsl("nothing at all"),
                                          QString::number(strongest, 'f', 2),
                                          tokens.page.name(),
                                          tokens.mutedText.name());

        QVERIFY2(extent >= 8, qPrintable(qsl("the handle is %1px wide, which is not room for a grip").arg(QString::number(extent))));
        // A floor rather than a measurement: six dots of two pixels across
        QVERIFY2(inked >= 6 * 4, qPrintable(qsl("the handle holds %1 painted pixels, so the six dots are not being drawn on it").arg(QString::number(inked))));
        QVERIFY2(strongest >= 0.7,
                 qPrintable(qsl("the grip's strongest pixel is %1, only %2 of the way to the quiet tone - it is a wash rather than the ink")
                                    .arg(strongestColour.name(), QString::number(strongest, 'f', 2))));
        QVERIFY2(dots.height() > dots.width(), qPrintable(qsl("the grip is %1x%2, which is not two columns of three dots").arg(QString::number(dots.width()), QString::number(dots.height()))));
    }

    void test_aColourWellIsCutToTheInputCorner()
    {
        // The well is on the options strip, which the editor opens closed
        mpEditor->setTriggerOptionsShown(true);
        QTest::qWait(50ms);

        QPushButton* pWell = mpEditor->mpTriggersMainArea->pushButtonFgColor;
        QVERIFY2(pWell != nullptr, "The trigger form has no foreground colour button");
        const QColor filled(0, 128, 0);
        pWell->setStyleSheet(dlgTriggerEditor::generateButtonStyleSheet(filled));
        QTest::qWait(50ms);
        QVERIFY2(pWell->isVisible() && pWell->width() > 8 && pWell->height() > 8, "The colour well is not on show, so there is nothing to measure");

        const QImage shot = TestPixels::shot(mpEditor);
        const auto at = [&](const QPoint& point) {
            return shot.pixelColor(pWell->mapTo(mpEditor, point));
        };
        const QColor fill = at(pWell->rect().center());
        const QColor corner = at(pWell->rect().topLeft());
        const QColor behind = at(QPoint(-2, pWell->height() / 2));
        qInfo().noquote() << qsl("  the well is filled %1, its corner is %2 and what it lies on is %3").arg(fill.name(), corner.name(), behind.name());

        QVERIFY2(fill.rgb() == filled.rgb(), qPrintable(qsl("the well is not filled with the colour it stands for: it is %1 where the colour is %2").arg(fill.name(), filled.name())));
        QVERIFY2(corner.rgb() != fill.rgb(), qPrintable(qsl("the well's corner is as solid as the rest of it, at %1 - it is a rectangle rather than a well").arg(corner.name())));
        QVERIFY2(corner.rgb() == behind.rgb(),
                 qPrintable(qsl("the well's corner is not cut away to what it lies on: the corner is %1 and the surface beside it %2").arg(corner.name(), behind.name())));
    }

    void test_theSoundFileClearButtonIsATintedGlyph()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        QToolButton* pClear = mpEditor->mpTriggersMainArea->toolButton_clearSoundFile;
        QVERIFY2(pClear != nullptr, "The trigger form has no button to empty the sound file field");
        // The button is only shown while the trigger has a sound file
        mpEditor->showTriggerSoundFile(qsl("/tmp/EditorChromeTest.wav"));
        QTest::qWait(50ms);
        QVERIFY2(!pClear->icon().isNull(), "The button carries no picture at all");

        // A fraction rather than a match, as a 1.5px stroke is antialiased;
        // qAbs, as on a dark page the words are lighter than the card
        const auto strongestInk = [&]() {
            const QImage shot = TestPixels::shot(mpEditor);
            qreal strongest = 0.0;
            for (int x = 0; x < pClear->width(); ++x) {
                for (int y = 0; y < pClear->height(); ++y) {
                    const QColor measured = shot.pixelColor(pClear->mapTo(mpEditor, QPoint(x, y)));
                    strongest = std::max(strongest, static_cast<qreal>(qAbs(measured.green() - tokens.card.green())) / std::max(1, qAbs(tokens.text.green() - tokens.card.green())));
                }
            }
            return strongest;
        };

        pClear->setEnabled(true);
        QTest::qWait(20ms);
        const qreal lively = strongestInk();
        pClear->setEnabled(false);
        QTest::qWait(20ms);
        const qreal quiet = strongestInk();
        qInfo().noquote() << qsl("  the glyph is drawn at %1px; its strongest pixel is %2 of the way from the card to the words switched on and %3 switched off")
                                     .arg(QString::number(pClear->iconSize().width()), QString::number(lively, 'f', 2), QString::number(quiet, 'f', 2));

        QCOMPARE(pClear->iconSize(), QSize(13, 13));
        QVERIFY2(lively > 0.5, qPrintable(qsl("the button's picture reaches only %1 of the way to the words, so it is not being drawn at all").arg(QString::number(lively, 'f', 2))));
        QVERIFY2(quiet < lively,
                 qPrintable(qsl("the button is drawn the same switched off as switched on - %1 against %2 - so it has no quiet tone")
                                    .arg(QString::number(quiet, 'f', 2), QString::number(lively, 'f', 2))));
    }

    void test_theSaveProfileMenuIsDrawnFromTheDesign()
    {
        QToolButton* pButton = saveProfileButton();
        QVERIFY2(pButton, "the Save Profile action has no button on the bar to hang a menu from");
        QMenu* pMenu = pButton->menu();
        QVERIFY2(pMenu, "the Save Profile button drops no menu");

        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QString sheets = sheetsReaching(pMenu);
        QVERIFY2(sheets.contains(qsl("QMenu")),
                 qPrintable(qsl("nothing over the Save Profile menu says what a QMenu is drawn like: the bar's sheet is \"%1\"").arg(mpEditor->toolBar->styleSheet().left(120))));
        QVERIFY2(sheets.contains(tokens.card.name()), qPrintable(qsl("the Save Profile menu is not drawn on the design's card surface (%1)").arg(tokens.card.name())));
    }

    void test_theSaveProfileMenuIsOpenAtTheCorner()
    {
        QToolButton* pButton = saveProfileButton();
        QVERIFY2(pButton, "the Save Profile action has no button on the bar to hang a menu from");
        QMenu* pMenu = pButton->menu();
        QVERIFY2(pMenu, "the Save Profile button drops no menu");
        QVERIFY2(pMenu->testAttribute(Qt::WA_TranslucentBackground), "the menu's window was never asked to be see-through, so it is opaque behind whatever corner the frame is given");

        const QImage shot = grabOfMenu(pMenu, underTheSaveProfileButton());
        QVERIFY2(shot.width() > 16 && shot.height() > 8, qPrintable(qsl("the menu came up %1x%2, which is nothing to read").arg(shot.width()).arg(shot.height())));

        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const qreal ratio = shot.devicePixelRatio();
        const int middle = shot.height() / 2;
        const QColor corner = QColor::fromRgba(shot.pixel(0, 0));
        // Halfway down, the left edge is a straight run of the hairline, and two
        // pixels in is the band the frame leaves round the rows
        const QColor hairline = QColor::fromRgba(shot.pixel(0, middle));
        const QColor surface = QColor::fromRgba(shot.pixel(qRound(2 * ratio), middle));
        QVERIFY2(corner.alpha() == 0, qPrintable(qsl("the menu's corner is painted %1 rather than cut away, so its radius has a square window behind it").arg(corner.name(QColor::HexArgb))));
        QVERIFY2(hairline.alpha() == 255 && TestPixels::distance(hairline, tokens.border) <= scmMarkInkSlack,
                 qPrintable(qsl("the menu's edge reads %1 rather than the design's hairline of %2").arg(hairline.name(QColor::HexArgb), tokens.border.name())));
        QVERIFY2(surface.alpha() == 255 && TestPixels::distance(surface, tokens.card) <= scmMarkInkSlack,
                 qPrintable(qsl("the menu is painted %1 rather than the card surface's %2").arg(surface.name(QColor::HexArgb), tokens.card.name())));
    }

    void test_theSaveProfileButtonShowsItsTwoHalves()
    {
        QToolButton* pButton = saveProfileButton();
        QVERIFY2(pButton, "the Save Profile action has no button on the bar to read");
        QCOMPARE(pButton->popupMode(), QToolButton::MenuButtonPopup);
        QVERIFY2(pButton->isVisible() && pButton->width() > 3 * uiDesign::scmInputDropDownWidth, "the Save Profile button is not on show wide enough to be read");

        const QString sheet = mpEditor->toolBar->styleSheet();
        const int arrowAt = sheet.indexOf(qsl("::menu-arrow"));
        QVERIFY2(arrowAt >= 0, "the bar's sheet says nothing about the chevron on the menu half, so the platform's filled triangle is still on it");
        QVERIFY2(sheet.mid(arrowAt, 80).contains(qsl("image:")), qPrintable(qsl("the menu half's chevron is named without a picture: \"%1\"").arg(sheet.mid(arrowAt, 80))));

        const QRect half = menuHalfOf(pButton);
        QVERIFY2(half.width() > 4 && half.height() > 8, qPrintable(qsl("the style hands the menu a %1x%2 rectangle, which is nothing to read").arg(half.width()).arg(half.height())));
        QVERIFY2(half.left() > pButton->width() / 2, "the half that opens the menu is not the trailing one");

        // Off the button first: an earlier case may have left it hovered
        pointAt(mpEditor->toolBar, QPoint(2, 2));
        const QImage rest = TestPixels::shot(mpEditor->toolBar);
        const QRect halfInGrab = inTheGrabsPixels(rest, QRect(pButton->mapTo(mpEditor->toolBar, half.topLeft()), half.size()));
        QVERIFY2(halfInGrab.width() > 4 && halfInGrab.height() > 8, "the menu half does not land inside the grab of the bar");

        // Clear of the ends, where the half's own corners round away from the line
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const int from = halfInGrab.top() + halfInGrab.height() / 5;
        const int to = halfInGrab.bottom() - halfInGrab.height() / 5;
        int onTheLine = 0;
        int rows = 0;
        QColor readAs;
        for (int y = from; y <= to; ++y) {
            ++rows;
            const QColor sample = rest.pixelColor(halfInGrab.left(), y);
            if (!readAs.isValid()) {
                readAs = sample;
            }
            if (TestPixels::distance(sample, tokens.border) <= scmMarkInkSlack) {
                ++onTheLine;
            }
        }
        // Control: a half filled in the hairline's tone would pass the reading above
        const int inside = halfInGrab.left() + qMax(3, qRound(3 * rest.devicePixelRatio()));
        const QColor justInside = rest.pixelColor(inside, halfInGrab.center().y() - halfInGrab.height() / 4);

        qInfo().noquote() << qsl("  the menu half is %1px wide at x %2; its leading edge reads %3 against the hairline's %4 on %5 of %6 rows, and %7 a few pixels in")
                                     .arg(half.width())
                                     .arg(half.left())
                                     .arg(readAs.name(), tokens.border.name())
                                     .arg(onTheLine)
                                     .arg(rows)
                                     .arg(justInside.name());
        QVERIFY2(rows > 2 && onTheLine * 5 >= rows * 4,
                 qPrintable(qsl("the half's leading edge carries no seam: %1 of %2 rows read the design's hairline of %3, the first of them %4")
                                    .arg(onTheLine)
                                    .arg(rows)
                                    .arg(tokens.border.name(), readAs.name())));
        QVERIFY2(TestPixels::distance(justInside, tokens.border) > scmMarkInkSlack,
                 qPrintable(qsl("the half is filled with the hairline's own tone (%1 a few pixels in), so what was read at its edge is a fill rather than a seam").arg(justInside.name())));

        // The wash band sits clear of the seam and the chevron; the face band sits
        // in the padding between the words and the seam, where nothing is drawn
        const int inset = qMax(3, qRound(3 * rest.devicePixelRatio()));
        const int bandHeight = qMax(2, halfInGrab.height() / 5);
        const QRect wash(halfInGrab.left() + inset, halfInGrab.top() + qMax(2, qRound(2 * rest.devicePixelRatio())), halfInGrab.width() - qMax(5, qRound(5 * rest.devicePixelRatio())), bandHeight);
        const QRect face(halfInGrab.left() - 2 * inset, wash.top(), inset, bandHeight);
        const QColor atRest = averageColour(rest, wash);

        // Which half the pointer is over is not asked: QStyleSheetStyle does not
        // answer hitTestComplexControl for CC_ToolButton, so the button reports
        // SC_ToolButton anywhere on it and no sheet can tell the halves apart
        pointAt(pButton, half.center());
        const QImage lit = TestPixels::shot(mpEditor->toolBar);
        const QColor litHalf = averageColour(lit, wash);
        const QColor litFace = averageColour(lit, face);

        const int faceToAccent = TestPixels::distance(litFace, tokens.accent);
        const int halfToAccent = TestPixels::distance(litHalf, tokens.accent);
        qInfo().noquote() << qsl("  the half reads %1 at rest and %2 pointed at, against the %3 of the face beside it; those two are %4 and %5 from the accent's %6")
                                     .arg(atRest.name(), litHalf.name(), litFace.name())
                                     .arg(halfToAccent)
                                     .arg(faceToAccent)
                                     .arg(tokens.accent.name());
        QVERIFY2(TestPixels::distance(litHalf, atRest) > 2,
                 qPrintable(qsl("pointing at the button changed nothing on the half (%1 against %2 at rest), so the pointer never reached it and nothing below can be read")
                                    .arg(litHalf.name(), atRest.name())));
        QVERIFY2(TestPixels::distance(litHalf, litFace) > 8,
                 qPrintable(qsl("the pointed-at half reads %1 against the %2 of the face beside it, so it is drawn as one piece with the body").arg(litHalf.name(), litFace.name())));
        QVERIFY2(halfToAccent < faceToAccent,
                 qPrintable(qsl("the pointed-at half is %1 from the accent against the %2 of the face beside it, so it does not light towards the accent").arg(halfToAccent).arg(faceToAccent)));

        pointAt(mpEditor->toolBar, QPoint(2, 2));
    }

    void test_theSearchOptionsMenuIsDrawnFromTheDesign()
    {
        QMenu* pMenu = searchOptionsMenu();
        QVERIFY2(pMenu, "the search options menu is not there to be read");
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QString sheets = sheetsReaching(pMenu);
        QVERIFY2(sheets.contains(qsl("QMenu")) && sheets.contains(tokens.card.name()),
                 qPrintable(qsl("the search options menu is not drawn on the design's card surface (%1)").arg(tokens.card.name())));
        QVERIFY2(sheets.contains(qsl("QMenu::indicator")), "nothing says what the mark on a checkable search option looks like, so the platform draws it");
        QVERIFY2(pMenu->testAttribute(Qt::WA_TranslucentBackground), "the search options menu's window was never asked to be see-through, so its corner is opaque");

        const QImage shot = grabOfMenu(pMenu, underTheSaveProfileButton());
        const QColor corner = QColor::fromRgba(shot.pixel(0, 0));
        QVERIFY2(corner.alpha() == 0, qPrintable(qsl("the search options menu's corner is painted %1 rather than cut away").arg(corner.name(QColor::HexArgb))));
    }

    void test_aCheckedSearchOptionCarriesTheOneMark()
    {
        QMenu* pMenu = searchOptionsMenu();
        QVERIFY2(pMenu, "the search options menu is not there to be read");
        QAction* pOption = mpEditor->mpAction_searchCaseSensitive;
        QVERIFY2(pOption && pOption->isCheckable(), "the case-sensitive search option is not a checkable row");

        const bool optionWas = pOption->isChecked();
        auto putItBack = qScopeGuard([pOption, optionWas]() {
            pOption->setChecked(optionWas);
        });

        pOption->setChecked(false);
        const QImage empty = grabOfMenu(pMenu, underTheSaveProfileButton());
        const QRect emptyBox = markBoxOf(pMenu, pOption, empty);
        QVERIFY2(emptyBox.width() > 4 && emptyBox.height() > 4, qPrintable(qsl("the option's mark has no rectangle to sample: %1x%2").arg(emptyBox.width()).arg(emptyBox.height())));
        const QColor fill = empty.pixelColor(emptyBox.center());
        const int nothingInIt = markedPixelsIn(empty, emptyBox, fill);

        pOption->setChecked(true);
        const QImage checked = grabOfMenu(pMenu, underTheSaveProfileButton());
        const int aTick = markedPixelsIn(checked, emptyBox, fill);

        // Against the fill the mark lies on rather than the accent framing the box
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QColor markInk = uiDesign::readableOn(tokens.field, tokens.accent, tokens.text, uiDesign::scmQuietMinimumRatio);
        int nearest = 3 * 255;
        for (int y = emptyBox.top(); y <= emptyBox.bottom(); ++y) {
            for (int x = emptyBox.left(); x <= emptyBox.right(); ++x) {
                nearest = std::min(nearest, TestPixels::distance(checked.pixelColor(x, y), markInk));
            }
        }

        qInfo().noquote() << qsl("  the option's mark is %1px on a fill of %2, with %3 pixels in it empty and %4 checked; its nearest ink to %5 is %6 away")
                                     .arg(emptyBox.width())
                                     .arg(fill.name())
                                     .arg(nothingInIt)
                                     .arg(aTick)
                                     .arg(markInk.name())
                                     .arg(nearest);
        QVERIFY2(nothingInIt == 0, qPrintable(qsl("an unchecked option already has %1 pixels in its box that are not the fill").arg(nothingInIt)));
        QVERIFY2(aTick > 0, "a checked option paints nothing inside its box, so there is no mark on it");
        QVERIFY2(nearest <= scmMarkInkSlack, qPrintable(qsl("the mark's nearest ink is %1 away from the %2 every other choice is marked in").arg(nearest).arg(markInk.name())));
    }
};

#include "EditorChromeTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorChromeTest)
