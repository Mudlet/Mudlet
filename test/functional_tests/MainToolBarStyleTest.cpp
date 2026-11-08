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
 * The main and detached toolbars use the flat-bar recipe in both appearances: left to the
 * platform, a hovered button shows nothing on native macOS and a checked one swallows its menu
 * half. A profile's Lua sheet lands on the same widget, so it has to come after the recipe.
 *
 * Run with: ctest -R MainToolBarStyleTest -V
 */

#include <QAction>
#include <QApplication>
#include <QHoverEvent>
#include <QHBoxLayout>
#include <QHash>
#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QToolBar>
#include <QToolButton>
#include <QScopeGuard>
#include <QtTest/QtTest>

#include "Host.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "TDetachedWindow.h"
#include "ToolBarOverflow.h"
#include "mudlet.h"
#include "uiDesign.h"

#include "GroupedTest.h"

class MainToolBarStyleTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    const QString mFirstHostname = qsl("MainToolBarStyle-First");
    const QString mSecondHostname = qsl("MainToolBarStyle-Second");

    // A wash is composited rather than blitted, so it agrees with the token mix to a level or two
    static constexpr int scmInkTolerance = 2;
    // Past the window's width Qt moves the bar's tail into the overflow menu, where nothing can be sampled
    static constexpr int scmWindowWidth = 1600;
    static constexpr int scmWindowHeight = 600;

    // Wide enough to hide nothing, and narrow enough to fold the bar's tail
    static constexpr int scmFoldingWidth = 760;

    static QToolBar* theBar() { return mudlet::self()->findChild<QToolBar*>(qsl("mpMainToolBar")); }

    static QToolButton* overflowButton(QToolBar* pBar) { return pBar->findChild<QToolButton*>(qsl("qt_toolbar_ext_button"), Qt::FindDirectChildrenOnly); }

    static int hiddenButtons(QToolBar* pBar)
    {
        int hidden = 0;
        for (QAction* pAction : pBar->actions()) {
            const QWidget* pWidget = pBar->widgetForAction(pAction);
            if (pWidget && pAction->isVisible() && !pAction->isSeparator() && pWidget->isHidden()) {
                ++hidden;
            }
        }
        return hidden;
    }

    static void windowWidth(const int width)
    {
        mudlet::self()->resize(width, scmWindowHeight);
        QTest::qWait(50);
    }

    // Tinting goes through premultiplied alpha, which can land a level off
    static bool sameInk(const QColor& first, const QColor& second)
    {
        return std::abs(first.red() - second.red()) <= scmInkTolerance && std::abs(first.green() - second.green()) <= scmInkTolerance && std::abs(first.blue() - second.blue()) <= scmInkTolerance;
    }

    // A down chevron is widest across the top of its ink, where its two arms start, and an up one across the bottom
    static bool pointsDown(const QIcon& icon, const QSize& size, const QIcon::State state)
    {
        const QImage image = icon.pixmap(size, QIcon::Normal, state).toImage().convertToFormat(QImage::Format_ARGB32);
        const auto spanOfRow = [&image](const int y) {
            int first = -1;
            int last = -1;
            for (int x = 0; x < image.width(); ++x) {
                if (image.pixelColor(x, y).alpha() > 64) {
                    first = first < 0 ? x : first;
                    last = x;
                }
            }
            return first < 0 ? -1 : last - first;
        };
        int top = 0;
        while (top < image.height() - 1 && spanOfRow(top) < 0) {
            ++top;
        }
        int bottom = image.height() - 1;
        while (bottom > 0 && spanOfRow(bottom) < 0) {
            --bottom;
        }
        return spanOfRow(top) > spanOfRow(bottom);
    }

    // The strongest pixel of the glyph, whose antialiased edge is lighter than its ink
    static QColor inkOf(const QIcon& icon, const QSize& size, const QIcon::State state)
    {
        const QImage image = icon.pixmap(size, QIcon::Normal, state).toImage().convertToFormat(QImage::Format_ARGB32);
        QColor strongest;
        int strongestAlpha = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (const QColor pixel = image.pixelColor(x, y); pixel.alpha() > strongestAlpha) {
                    strongestAlpha = pixel.alpha();
                    strongest = pixel;
                }
            }
        }
        return strongest;
    }

    static QToolButton* buttonNamed(const QString& objectName) { return theBar() ? theBar()->findChild<QToolButton*>(objectName) : nullptr; }

    static QString saidAs(const QColor& colour) { return colour.name(QColor::HexArgb); }

    static QString saidAs(const QPoint& at) { return qsl("%1,%2").arg(QString::number(at.x()), QString::number(at.y())); }

    // Past the rounded corner and above the glyph and the word, where only the fill is drawn
    static QPoint washSpotIn(const QRect& button) { return QPoint(button.left() + uiDesign::scmToolBarButtonRadius + 2, button.top() + 2); }

    // splitButtonMenuHalfStyleSheet() draws the seam on the leading edge of a scmInputDropDownWidth
    // menu half, inside the one-pixel border the bar's rule gives every button
    static int seamColumnOf(const QRect& button) { return button.right() - uiDesign::scmInputDropDownWidth; }

    // QStyleOption::initFrom() takes State_MouseOver from WA_UnderMouse, which QApplication sets
    // only while dispatching a real enter or leave, so the offscreen platform never sets it
    static void hoverOver(QWidget* pWidget, const bool over)
    {
        const QPoint at = pWidget->rect().center();
        if (over) {
            QTest::mouseMove(pWidget, at);
        }
        QCoreApplication::processEvents();
        pWidget->setAttribute(Qt::WA_UnderMouse, over);
        if (over) {
            // Named first: GCC reads entered(QPointF(at), QPointF(at), ...) as a declaration with two parameters called at
            const QPointF local(at);
            QEnterEvent entered(local, local, QPointF(pWidget->mapToGlobal(at)));
            QApplication::sendEvent(pWidget, &entered);
            QHoverEvent hovered(QEvent::HoverEnter, local, local, QPointF(-1, -1));
            QApplication::sendEvent(pWidget, &hovered);
        } else {
            QEvent left(QEvent::Leave);
            QApplication::sendEvent(pWidget, &left);
            QHoverEvent unhovered(QEvent::HoverLeave, QPointF(-1, -1), QPointF(-1, -1), QPointF(at));
            QApplication::sendEvent(pWidget, &unhovered);
        }
        pWidget->update();
        QCoreApplication::processEvents();
    }

    static QString appearanceSaidAs(const enums::Appearance appearance) { return appearance == enums::Appearance::dark ? qsl("dark") : qsl("light"); }

    void aHoveredButtonIsWashed(const QString& appearance)
    {
        QToolButton* pHovered = buttonNamed(qsl("triggers_action"));
        QToolButton* pResting = buttonNamed(qsl("aliases_action"));
        QVERIFY2(pHovered && pResting, "the bar has no Triggers or Aliases button to point at");
        QVERIFY2(pHovered->isVisible() && pResting->isVisible(), "the window is too narrow for the bar, so the buttons this case reads are in the overflow menu rather than on it");

        hoverOver(pHovered, true);
        const QImage bar = TestPixels::shot(theBar());
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();

        const QColor washed = TestPixels::sampled(bar, washSpotIn(pHovered->geometry()));
        const QColor wanted = TestPixels::composited(tokens.hoverWash, tokens.page);
        const QColor resting = TestPixels::sampled(bar, washSpotIn(pResting->geometry()));
        hoverOver(pHovered, false);

        qInfo().noquote() << qsl("  %1: the hovered button reads %2 against an expected %3, and one at rest %4 against the bar's %5")
                                     .arg(appearance, saidAs(washed), saidAs(wanted), saidAs(resting), saidAs(tokens.page));

        QVERIFY2(TestPixels::readsAs(washed, wanted, scmInkTolerance),
                 qPrintable(qsl("%1: a button under the pointer is not washed in the hover tone: %2 at %3 where the wash over the bar's own surface is %4")
                                    .arg(appearance, saidAs(washed), saidAs(washSpotIn(pHovered->geometry())), saidAs(wanted))));
        QVERIFY2(
                TestPixels::readsAs(resting, tokens.page, scmInkTolerance),
                qPrintable(qsl("%1: a button nothing is pointing at is drawn on something other than the bar's own surface: %2 rather than %3").arg(appearance, saidAs(resting), saidAs(tokens.page))));
    }

    void aSwitchedOnSplitButtonReadsAsTwoAreas(const QString& appearance)
    {
        QToolButton* pButton = buttonNamed(qsl("mute"));
        QAction* pAction = mudlet::self()->findChild<QAction*>(qsl("muteMedia"));
        QVERIFY2(pButton && pAction, "the bar has no Sound button, or no muteMedia action behind it");
        QVERIFY2(pButton->isVisible(), "the window is too narrow for the bar, so the Sound button is in the overflow menu rather than on it");
        QVERIFY2(pButton->popupMode() == QToolButton::MenuButtonPopup, "the Sound button no longer keeps a menu half, so there is nothing here to tell from its body");

        pAction->setChecked(true);
        QCoreApplication::processEvents();
        QVERIFY2(pButton->isChecked(), "checking the mute action did not switch the button it is the default action of on");

        const QImage bar = TestPixels::shot(theBar());
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QRect box = pButton->geometry();

        const QColor body = TestPixels::sampled(bar, washSpotIn(box));
        const QColor wantedBody = TestPixels::composited(tokens.accentWash, tokens.page);

        // The seam is one pixel wide and rounding the sub-control's box can move it a pixel
        const int seamAt = seamColumnOf(box);
        QColor seam;
        int seamFoundAt = -1;
        QStringList aroundTheSeam;
        for (int x = seamAt - 1; x <= seamAt + 1; ++x) {
            const QColor read = TestPixels::sampled(bar, QPoint(x, box.center().y()));
            aroundTheSeam << qsl("%1:%2").arg(QString::number(x), saidAs(read));
            if (seamFoundAt < 0 && TestPixels::readsAs(read, tokens.border, scmInkTolerance)) {
                seam = read;
                seamFoundAt = x;
            }
        }

        pAction->setChecked(false);
        QCoreApplication::processEvents();

        qInfo().noquote() << qsl("  %1: the switched-on Sound button reads %2 against an expected %3, with the seam wanted at x=%4 in %5 and the columns around it %6")
                                     .arg(appearance, saidAs(body), saidAs(wantedBody), QString::number(seamAt), saidAs(tokens.border), aroundTheSeam.join(qsl(", ")));

        QVERIFY2(TestPixels::readsAs(body, wantedBody, scmInkTolerance),
                 qPrintable(qsl("%1: a switched-on button on the bar is not washed in the accent: %2 at %3 where the accent's wash over the bar's surface is %4")
                                    .arg(appearance, saidAs(body), saidAs(washSpotIn(box)), saidAs(wantedBody))));
        QVERIFY2(seamFoundAt >= 0,
                 qPrintable(qsl("%1: the Sound button's menu half is not told from its body at all - the seam the recipe draws in %2 is nowhere within a pixel of x=%3 on a button at %4; the "
                                "columns there read %5")
                                    .arg(appearance, saidAs(tokens.border), QString::number(seamAt), TestPixels::describe(box), aroundTheSeam.join(qsl(", ")))));
        QVERIFY2(!TestPixels::readsAs(seam, wantedBody, scmInkTolerance),
                 qPrintable(qsl("%1: the seam on the Sound button's menu half is the same wash its body is, %2, so the two halves read as one target").arg(appearance, saidAs(seam))));
    }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.startMudlet(), qPrintable(mSession.why()));
        mudlet::self()->setInterfaceLanguage(qsl("en_US"));

        // Two of them, because slot_tabDetachRequested() refuses index 0
        QVERIFY2(mSession.openProfile(mFirstHostname), qPrintable(mSession.why()));
        QVERIFY2(mSession.openProfile(mSecondHostname), qPrintable(mSession.why()));

        // With a profile loaded the shipped default hides the bar, and a hidden bar cannot be sampled
        mudlet::self()->setToolBarVisibility(enums::visibleAlways);
        mudlet::self()->resize(scmWindowWidth, scmWindowHeight);
        mudlet::self()->show();
        QVERIFY(QTest::qWaitForWindowExposed(mudlet::self()));
        QCoreApplication::processEvents();

        QVERIFY2(theBar(), "the main window has no toolbar named mpMainToolBar");
        QVERIFY2(theBar()->isVisible() && theBar()->width() > 0, "the main window's toolbar is not on show, so nothing below can be read off a grab of it");
    }

    void cleanupTestCase()
    {
        if (mudlet::self()) {
            mudlet::self()->setAppearance(enums::Appearance::systemSetting);
        }
        mSession.stop();
    }

    void test_theBarCarriesTheRecipeAndTheProfilesSheetComesLast()
    {
        const QString sheet = theBar()->styleSheet();
        QVERIFY2(sheet.contains(qsl("QToolBar#mpMainToolBar {")), qPrintable(qsl("the bar's sheet does not draw the bar itself: %1").arg(sheet.left(200))));
        QVERIFY2(sheet.contains(qsl("QToolButton:hover")), "the bar's sheet says nothing about a button under the pointer, so the platform style draws that");
        QVERIFY2(sheet.contains(qsl("QToolButton:checked")), "the bar's sheet says nothing about a button that is switched on, so Sound and Full Screen are drawn by the platform style");
        QVERIFY2(sheet.contains(qsl("[popupMode=\"1\"]")), "the bar's sheet never picks out the buttons whose trailing half opens a menu");
        QVERIFY2(sheet.contains(qsl("::menu-button")), "the bar's sheet draws no menu half on a split button, so its two click areas read as one");

        Host* pHost = mudlet::self()->getActiveHost();
        QVERIFY2(pHost, "no profile is active, so no profile stylesheet can reach the bar");
        const QString marker = qsl("/* profile-sheet-marker */");
        pHost->setProfileStyleSheet(marker);
        const QString composed = theBar()->styleSheet();
        pHost->setProfileStyleSheet(QString());

        QVERIFY2(composed.endsWith(marker),
                 qPrintable(qsl("a profile's own stylesheet does not come last on the bar, so it cannot override the design's rules on the widget they share: the sheet ends \"%1\"")
                                    .arg(composed.right(120))));
        QVERIFY2(composed.contains(qsl("QToolBar#mpMainToolBar {")), "a profile's stylesheet replaced the design's rules on the bar rather than being composed after them");
    }

    void test_aScriptSheetNamingTheBarStylesIt_data()
    {
        QTest::addColumn<bool>("application");
        QTest::newRow("setAppStyleSheet") << true;
        QTest::newRow("setProfileStyleSheet") << false;
    }

    // As scripts in the package repository restyle the bar: the design's rules on the bar itself would outrank an
    // application sheet whatever its selectors, and a profile sheet's plain type selectors
    void test_aScriptSheetNamingTheBarStylesIt()
    {
        QFETCH(bool, application);
        Host* pHost = mudlet::self()->getActiveHost();
        QVERIFY2(pHost, "no profile is active, so no profile stylesheet can reach the bar");
        const QColor background(1, 2, 3);
        const QColor word(250, 240, 230);
        const QString sheet = qsl("QToolBar { background-color: %1; } QToolButton { color: %2; }").arg(background.name(), word.name());
        const auto restore = qScopeGuard([pHost]() {
            mudlet::self()->setAppStyleSheet(QString());
            pHost->setProfileStyleSheet(QString());
        });
        if (application) {
            mudlet::self()->setAppStyleSheet(sheet);
        } else {
            pHost->setProfileStyleSheet(sheet);
        }
        QCoreApplication::processEvents();

        const QImage picture = theBar()->grab().toImage().convertToFormat(QImage::Format_ARGB32);
        int filled = 0;
        int worded = 0;
        for (int y = 0; y < picture.height(); ++y) {
            for (int x = 0; x < picture.width(); ++x) {
                const QColor pixel = picture.pixelColor(x, y);
                filled += pixel.rgb() == background.rgb() ? 1 : 0;
                worded += pixel.rgb() == word.rgb() ? 1 : 0;
            }
        }
        const int area = picture.width() * picture.height();
        QVERIFY2(filled > area / 2, qPrintable(qsl("%1 of the bar's %2 pixels are the sheet's background").arg(QString::number(filled), QString::number(area))));
        QVERIFY2(worded > 20, qPrintable(qsl("%1 pixels are in the sheet's word colour, so the buttons' names are not").arg(QString::number(worded))));
    }

    void test_aHoveredButtonIsWashed() { aHoveredButtonIsWashed(qsl("as started")); }

    void test_aSwitchedOnSplitButtonReadsAsTwoAreas() { aSwitchedOnSplitButtonReadsAsTwoAreas(qsl("as started")); }

    void test_bothAppearances()
    {
        for (const enums::Appearance appearance : {enums::Appearance::dark, enums::Appearance::light}) {
            TestPixels::setAppearanceAndWait(appearance);
            aHoveredButtonIsWashed(appearanceSaidAs(appearance));
            if (QTest::currentTestFailed()) {
                break;
            }
            aSwitchedOnSplitButtonReadsAsTwoAreas(appearanceSaidAs(appearance));
            if (QTest::currentTestFailed()) {
                break;
            }
        }
        TestPixels::setAppearanceAndWait(enums::Appearance::systemSetting);
    }

    void test_aNarrowWindowShowsHowManyButtonsAreHidden()
    {
        QToolBar* pBar = theBar();
        QToolButton* pMore = overflowButton(pBar);
        QVERIFY2(pMore, "the bar has no overflow button");
        const auto restore = qScopeGuard([]() {
            windowWidth(scmWindowWidth);
        });

        windowWidth(scmWindowWidth);
        QVERIFY2(!pMore->isVisible(), "the overflow button is on show on a window every button fits in");

        for (const enums::Appearance appearance : {enums::Appearance::dark, enums::Appearance::light}) {
            TestPixels::setAppearanceAndWait(appearance);
            const QString said = appearanceSaidAs(appearance);
            windowWidth(scmFoldingWidth);
            QVERIFY2(pMore->isVisible(), qPrintable(qsl("nothing folded at %1px in %2").arg(QString::number(scmFoldingWidth), said)));

            const int hidden = hiddenButtons(pBar);
            QVERIFY2(hidden > 0, "the overflow button shows with no button hidden");
            QCOMPARE(pMore->text(), uiDesign::ToolBarOverflow::word(hidden));
            QCOMPARE(pMore->toolButtonStyle(), pBar->toolButtonStyle());
            QCOMPARE(pMore->iconSize(), pBar->iconSize());

            // Through the proxy style and the layout, or the word would be cut off
            QCOMPARE(pMore->width(), pBar->property(uiDesign::scmProp_toolBarOverflowExtent).toInt());
            QVERIFY2(pMore->width() >= pMore->QToolButton::sizeHint().width(), qPrintable(qsl("%1px is too narrow for \"%2\" in %3").arg(QString::number(pMore->width()), pMore->text(), said)));

            const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
            const QColor closedInk = inkOf(pMore->icon(), pMore->iconSize(), QIcon::Off);
            const QColor openInk = inkOf(pMore->icon(), pMore->iconSize(), QIcon::On);
            QVERIFY2(sameInk(closedInk, tokens.mutedText), qPrintable(qsl("the closed chevron is %1 where the chrome is %2 in %3").arg(closedInk.name(), tokens.mutedText.name(), said)));
            QVERIFY2(sameInk(openInk, tokens.accentText), qPrintable(qsl("the open chevron is %1 where the accent ink is %2 in %3").arg(openInk.name(), tokens.accentText.name(), said)));
            QVERIFY2(pointsDown(pMore->icon(), pMore->iconSize(), QIcon::Off), "the closed chevron does not point down");
            QVERIFY2(!pointsDown(pMore->icon(), pMore->iconSize(), QIcon::On), "the open chevron does not point up");

            // Wider, not narrower: the window's minimum width can sit just under the folding width
            int wider = scmFoldingWidth;
            int fewer = hidden;
            while (fewer == hidden && wider < scmWindowWidth) {
                wider += 20;
                windowWidth(wider);
                fewer = hiddenButtons(pBar);
            }
            QVERIFY2(fewer > 0 && fewer < hidden,
                     qPrintable(qsl("widening from %1px to %2px took the hidden buttons from %3 to %4, so the count below proves nothing")
                                        .arg(QString::number(scmFoldingWidth), QString::number(wider), QString::number(hidden), QString::number(fewer))));
            QCOMPARE(pMore->text(), uiDesign::ToolBarOverflow::word(fewer));
        }
        TestPixels::setAppearanceAndWait(enums::Appearance::systemSetting);

        windowWidth(scmWindowWidth);
        QVERIFY2(!pMore->isVisible(), "the overflow button stayed after the window grew back");
    }

    // Only macOS gives QToolButton a font of its own, so this makes one for the other platforms
    void test_theWordIsTheSizeOfTheBarsOwnButtons()
    {
        QToolBar* pBar = theBar();
        QToolButton* pMore = overflowButton(pBar);
        QVERIFY2(pMore, "the bar has no overflow button");
        const QFont buttonFont = QApplication::font("QToolButton");
        const auto restore = qScopeGuard([&buttonFont]() {
            QApplication::setFont(buttonFont, "QToolButton");
            mudlet::self()->restyleMainToolBar();
            windowWidth(scmWindowWidth);
        });

        QFont smaller = buttonFont;
        smaller.setPointSizeF(buttonFont.pointSizeF() - 3);
        QApplication::setFont(smaller, "QToolButton");
        mudlet::self()->restyleMainToolBar();
        windowWidth(scmFoldingWidth);
        QVERIFY2(pMore->isVisible(), "nothing folded, so the size below proves nothing");

        QTRY_COMPARE(buttonNamed(qsl("triggers_action"))->font().pointSizeF(), smaller.pointSizeF());
        QTRY_COMPARE(pMore->font().pointSizeF(), smaller.pointSizeF());
        QVERIFY2(pMore->width() >= pMore->QToolButton::sizeHint().width(), "the bar did not fit the word to its smaller size");
    }

    // Qt leaves the row too long when the button it hid to make room is narrower than the overflow button, and which
    // ones those are depends on the platform's font, so those widths prove nothing here. The sheet's rule keeps them few.
    void test_foldingLeavesTheGapsBetweenTheButtonsAlone()
    {
        QToolBar* pBar = theBar();
        QLayout* pLayout = pBar->layout();
        const auto restore = qScopeGuard([]() {
            windowWidth(scmWindowWidth);
        });
        windowWidth(scmWindowWidth);
        QVERIFY2(pBar->styleSheet().contains(qsl("QToolBarExtension { padding-left: 0px; padding-right: 0px; }")), "the overflow button has its side padding back");

        QHash<const QWidget*, int> fullWidths;
        for (int i = 0; i < pLayout->count(); ++i) {
            // A button whose action is switched off is hidden on every width
            if (const QWidget* pItem = pLayout->itemAt(i)->widget(); pItem && pItem->isVisible()) {
                fullWidths.insert(pItem, pItem->width());
            }
        }

        int foldedWidths = 0;
        for (int width = scmWindowWidth; width > 0; width -= 2) {
            mudlet::self()->resize(width, scmWindowHeight);
            // The bar lays out on the posted resize and layout request; no timer is involved
            QCoreApplication::sendPostedEvents();
            QCoreApplication::processEvents();
            if (mudlet::self()->width() != width) {
                break;
            }
            QWidget* pPrevious = nullptr;
            QStringList gaps;
            bool hiddenNarrower = false;
            for (int i = 0; i < pLayout->count(); ++i) {
                QWidget* pItem = pLayout->itemAt(i)->widget();
                if (!fullWidths.contains(pItem)) {
                    continue;
                }
                if (pItem->isHidden()) {
                    hiddenNarrower = fullWidths.value(pItem) < pBar->property(uiDesign::scmProp_toolBarOverflowExtent).toInt() + pLayout->spacing();
                    break;
                }
                if (pPrevious) {
                    if (const int gap = pItem->x() - pPrevious->geometry().right() - 1; gap != pLayout->spacing()) {
                        gaps << qsl("%1px after \"%2\"").arg(QString::number(gap), pPrevious->objectName());
                    }
                }
                pPrevious = pItem;
            }
            if (hiddenNarrower) {
                continue;
            }
            QVERIFY2(gaps.isEmpty(), qPrintable(qsl("%1 on a %2px window, not %3px").arg(gaps.join(qsl(", ")), QString::number(width), QString::number(pLayout->spacing()))));
            foldedWidths += overflowButton(pBar)->isVisible() ? 1 : 0;
        }
        QVERIFY2(foldedWidths > 0, "no width folded the bar on a button wider than the overflow button, so the gaps above prove nothing");
    }

    // The notepad's note is a bare QWidget with two labels in it, the same class as a spacer
    void test_theCountLeavesOutASpacerAndKeepsAGroupOfWidgets()
    {
        QMainWindow window;
        auto* pBar = new QToolBar(&window);
        window.addToolBar(pBar);
        pBar->addAction(qsl("first"));
        pBar->addAction(qsl("second"));
        auto* pSpacer = new QWidget(pBar);
        pSpacer->setFixedWidth(40);
        pBar->addWidget(pSpacer);
        auto* pGroup = new QWidget(pBar);
        auto* pGroupLayout = new QHBoxLayout(pGroup);
        pGroupLayout->addWidget(new QLabel(qsl("a note that takes up some room"), pGroup));
        pBar->addWidget(pGroup);

        uiDesign::ToolBarOverflow::restyle(pBar, uiDesign::themeTokens());
        window.resize(pBar->sizeHint().width() + 200, 200);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        window.resize(1, 200);
        QTest::qWait(50);
        QVERIFY2(pGroup->isHidden() && pSpacer->isHidden(), "the narrowest window left the tail of the bar on show, so the count below proves nothing");
        int hiddenTools = 0;
        for (QToolButton* pButton : pBar->findChildren<QToolButton*>(QString(), Qt::FindDirectChildrenOnly)) {
            hiddenTools += pButton != overflowButton(pBar) && pButton->isHidden() ? 1 : 0;
        }
        QCOMPARE(overflowButton(pBar)->text(), uiDesign::ToolBarOverflow::word(hiddenTools + 1));
    }

    void test_theFoldedButtonsOpenInARowAndTheCountHolds()
    {
        QToolBar* pBar = theBar();
        QToolButton* pMore = overflowButton(pBar);
        const auto restore = qScopeGuard([pMore]() {
            if (pMore->isChecked()) {
                pMore->click();
            }
            windowWidth(scmWindowWidth);
        });
        windowWidth(scmFoldingWidth);
        QVERIFY2(pMore->popupMode() != QToolButton::InstantPopup, "this window's height left Qt offering a menu rather than a row");
        const QString closedWord = pMore->text();
        const QString closedTip = pMore->toolTip();
        const int hidden = hiddenButtons(pBar);
        QVERIFY(hidden > 0);

        pMore->click();

        QVERIFY2(pMore->isChecked(), "the button did not stay lit while the row is open");
        // QMainWindow animates the bar open, and lays the row out as that ends
        QTRY_COMPARE(hiddenButtons(pBar), 0);
        QTest::qWait(50);
        QCOMPARE(pMore->text(), closedWord);
        QVERIFY2(pMore->toolTip() != closedTip, "the tooltip still offers to show buttons that are on show");

        pMore->click();
        QVERIFY(!pMore->isChecked());
        QTRY_COMPARE(hiddenButtons(pBar), hidden);
        QTest::qWait(50);
        QCOMPARE(pMore->text(), closedWord);
        QCOMPARE(pMore->toolTip(), closedTip);
    }

    void test_theWordFollowsTheToolbarIconSize()
    {
        QToolBar* pBar = theBar();
        QToolButton* pMore = overflowButton(pBar);
        const int was = mudlet::self()->mToolbarIconSize;
        const auto restore = qScopeGuard([was]() {
            mudlet::self()->setToolBarIconSize(was);
            windowWidth(scmWindowWidth);
        });
        windowWidth(scmFoldingWidth);

        mudlet::self()->setToolBarIconSize(2);
        QTest::qWait(50);
        QCOMPARE(pMore->toolButtonStyle(), Qt::ToolButtonIconOnly);
        QCOMPARE(pMore->iconSize(), pBar->iconSize());
        const int iconOnlyWidth = pBar->property(uiDesign::scmProp_toolBarOverflowExtent).toInt();

        mudlet::self()->setToolBarIconSize(3);
        QTest::qWait(50);
        QCOMPARE(pMore->toolButtonStyle(), Qt::ToolButtonTextUnderIcon);
        QCOMPARE(pMore->iconSize(), pBar->iconSize());
        QVERIFY2(pBar->property(uiDesign::scmProp_toolBarOverflowExtent).toInt() > iconOnlyWidth, "the button left no more room for a word than for a glyph alone");
    }

    void test_coloredIconsTakeTheColorsOfTheOldPictures()
    {
        auto restore = qScopeGuard([]() {
            mudlet::self()->setColouredIcons(false);
            TestPixels::setAppearanceAndWait(enums::Appearance::systemSetting);
        });
        const auto icon = [](const QString& objectName) {
            const QAction* pAction = mudlet::self()->findChild<QAction*>(objectName);
            return pAction ? pAction->icon().pixmap(QSize(64, 64), QIcon::Normal, QIcon::Off).toImage().convertToFormat(QImage::Format_ARGB32) : QImage();
        };
        // The most saturated of the glyph's solid pixels, and whether any of its pixels is a part-clear wash
        struct Read
        {
            qreal hue = -1.0;
            qreal saturation = 0.0;
            bool washed = false;
        };
        const auto read = [](const QImage& image) {
            Read seen;
            for (int y = 0; y < image.height(); ++y) {
                for (int x = 0; x < image.width(); ++x) {
                    const QColor pixel = image.pixelColor(x, y);
                    if (pixel.alphaF() > 0.95 && pixel.hslSaturationF() > seen.saturation) {
                        seen.saturation = pixel.hslSaturationF();
                        seen.hue = pixel.hslHueF();
                    }
                    seen.washed = seen.washed || (pixel.alphaF() > 0.3 && pixel.alphaF() < 0.6);
                }
            }
            return seen;
        };

        for (const enums::Appearance appearance : {enums::Appearance::dark, enums::Appearance::light}) {
            TestPixels::setAppearanceAndWait(appearance);
            const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
            const QString said = appearanceSaidAs(appearance);
            QVERIFY2(read(icon(qsl("map_action"))).saturation < 0.1, qPrintable(qsl("%1: Map is in color with the switch off").arg(said)));

            mudlet::self()->setColouredIcons(true);
            const Read map = read(icon(qsl("map_action")));
            QVERIFY2(map.saturation > 0.3 && std::abs(map.hue - uiDesign::scmIdentity_map.hue) < 0.03,
                     qPrintable(qsl("%1: Map's line is not the blue of the old globe: hue %2 at saturation %3, wanted hue %4")
                                        .arg(said, QString::number(map.hue), QString::number(map.saturation), QString::number(uiDesign::scmIdentity_map.hue))));
            QVERIFY2(map.washed, qPrintable(qsl("%1: Map's shapes carry no wash of its color under the line").arg(said)));
            for (const QString& grey : {qsl("settings_action"), qsl("about_action")}) {
                QVERIFY2(read(icon(grey)).saturation < 0.1, qPrintable(qsl("%1: %2 took a color, though its old picture was grey or its green means an update").arg(said, grey)));
            }
            for (const uiDesign::IdentityColour& identity : {uiDesign::scmIdentity_connect,
                                                             uiDesign::scmIdentity_triggers,
                                                             uiDesign::scmIdentity_buttons,
                                                             uiDesign::scmIdentity_variables,
                                                             uiDesign::scmIdentity_sound,
                                                             uiDesign::scmIdentity_discord,
                                                             uiDesign::scmIdentity_map,
                                                             uiDesign::scmIdentity_manual,
                                                             uiDesign::scmIdentity_packages,
                                                             uiDesign::scmIdentity_reconnect}) {
                const QColor ink = uiDesign::identityInk(identity, tokens);
                QVERIFY2(uiDesign::contrastRatio(ink, tokens.page) >= uiDesign::scmQuietMinimumRatio && uiDesign::contrastRatio(ink, tokens.card) >= uiDesign::scmQuietMinimumRatio,
                         qPrintable(qsl("%1: the line for hue %2 is %3, under 3:1 on the bar or on a menu").arg(said, QString::number(identity.hue), ink.name())));
            }

            mudlet::self()->setColouredIcons(false);
            QVERIFY2(read(icon(qsl("map_action"))).saturation < 0.1, qPrintable(qsl("%1: Map kept its color after the switch went off").arg(said)));
        }
    }

    // Their lines leave the middle open, so a fill of the lines alone has a gap across it
    void test_aGlyphWithAnOpenMiddleIsFilledByItsOwnShape()
    {
        const QPixmap reconnect = uiDesign::glyphFillPixmap(qsl(":/icons/toolbar-reconnect.svg"));
        const QPixmap variables = uiDesign::glyphFillPixmap(qsl(":/icons/editor-variables.svg"));
        QVERIFY(!reconnect.isNull() && !variables.isNull());
        const qreal unit = uiDesign::scmGlyphRasterSize / 24.0;
        const auto alphaAt = [unit](const QPixmap& pixmap, const qreal x, const qreal y) {
            return pixmap.toImage().pixelColor(qRound(x * unit), qRound(y * unit)).alphaF();
        };
        QVERIFY2(alphaAt(reconnect, 12, 12) > 0.9, "Reconnect's wash leaves the middle of its circle open");
        QVERIFY2(alphaAt(variables, 12, 7) > 0.9, "Variables' wash leaves the space between its braces open");
        // Above the chord the arc's own ends would close
        QVERIFY2(alphaAt(uiDesign::glyphFillPixmap(qsl(":/icons/editor-activate.svg")), 12, 5) > 0.9, "Activate's wash leaves the top of its circle open");
        QVERIFY2(uiDesign::glyphFillPixmap(qsl(":/icons/toolbar-discord.png")).isNull(), "a raster mark, already a filled shape, was given a wash as well");
    }

    void test_theAppearancePalettesDotsTakeAPaintEach()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        // Grey in the settings sidebar, as Appearance takes no colour of its own
        QPixmap painted = uiDesign::tintedGlyph(uiDesign::glyphPixmap(qsl(":/icons/settings-appearance.svg")), tokens.mutedText);
        uiDesign::addPaintDots(painted, qsl(":/icons/settings-appearance.svg"), tokens);
        const QImage palette = painted.toImage();
        const qreal unit = palette.width() / 24.0;
        // The dots' centres in settings-appearance-dots.svg, in the order of their ids
        const QList<QPointF> centres{{13.5, 6.5}, {17.5, 10.5}, {6.5, 12.5}, {8.5, 7.5}};
        QStringList read;
        for (int dot = 0; dot < centres.size(); ++dot) {
            const QColor pixel = palette.pixelColor((centres.at(dot) * unit).toPoint());
            const qreal wanted = uiDesign::scmIdentityPaintDots[dot].hue;
            const qreal distance = std::min(std::abs(pixel.hslHueF() - wanted), 1.0 - std::abs(pixel.hslHueF() - wanted));
            read << qsl("dot %1 %2").arg(QString::number(dot), pixel.name());
            QVERIFY2(pixel.alphaF() > 0.95 && distance < 0.04,
                     qPrintable(qsl("the palette's dot %1 is %2, not the paint it was given: %3").arg(QString::number(dot), pixel.name(), read.join(qsl(", ")))));
        }
    }

    void test_aDetachedWindowsBarCarriesTheRecipe()
    {
        QVERIFY2(mudlet::self()->getDetachedWindows().isEmpty(), "a detached window was left over from an earlier test");
        mudlet::self()->slot_tabDetachRequested(1, QPoint(200, 200));
        TDetachedWindow* pDetachedWindow = mudlet::self()->getDetachedWindows().value(mSecondHostname);
        QVERIFY2(pDetachedWindow, qPrintable(qsl("detaching tab 1 produced no window for '%1' - the tab order is not what this test assumes").arg(mSecondHostname)));

        QToolBar* pDetachedBar = pDetachedWindow->findChild<QToolBar*>(qsl("detachedMainToolBar"));
        const QString sheet = pDetachedBar ? pDetachedBar->styleSheet() : QString();
        QTest::qWait(20);
        const int overflowExtent = pDetachedBar ? pDetachedBar->property(uiDesign::scmProp_toolBarOverflowExtent).toInt() : 0;
        mudlet::self()->slot_tabReattachRequested(mSecondHostname);

        QVERIFY2(pDetachedBar, "the detached window has no toolbar named detachedMainToolBar");
        QVERIFY2(sheet.contains(qsl("QToolBar#detachedMainToolBar {")), qPrintable(qsl("a detached window's bar is left for the platform style to draw: %1").arg(sheet.left(200))));
        QVERIFY2(sheet.contains(qsl("QToolButton:checked")), "a detached window's bar says nothing about a button that is switched on, so its Sound button is drawn by the platform style");
        QVERIFY2(overflowExtent > 0, "a detached window's overflow button was not given the room for its word");
    }
};

#include "MainToolBarStyleTest.moc"
MUDLET_GROUPED_TEST_MAIN(MainToolBarStyleTest)
