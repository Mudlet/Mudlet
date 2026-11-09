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
 * A QToolBar too narrow for its actions hides the tail, Save Profile among them,
 * behind a chevron a few pixels wide. The editor's bar gives its names up
 * instead; these cases hold every action on it at every width the window reaches.
 */

#include <QLayout>
#include <QMainWindow>
#include <QToolBar>
#include <QToolButton>
#include <QScopeGuard>
#include <QHash>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostDialogs.h"
#include "ProfileSessionTestHelper.h"
#include "ToolBarOverflow.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"
#include "uiDesign.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class EditorToolBarOverflowTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("EditorToolBarOverflow-Test-Profile");

    // Indexes into mEditorToolBarGroups, whose order is also the order names are given up in
    static constexpr int scmProfileGroup = 0;
    static constexpr int scmItemGroup = 1;

    // 1600px fits every name at any icon size; 8px places a breakpoint within a
    // button's width
    static constexpr int scmSweepWidest = 1600;
    static constexpr int scmSweepStep = 8;
    static constexpr int scmSweepHeight = 700;
    static constexpr int scmDefaultPreference = 3;
    static constexpr int scmLargestPreference = 4;

    // QToolBar's overflow button exists from the start and is shown only while
    // something is folded into it, so its visibility is the fold itself
    QWidget* extensionButton() const { return mpEditor->toolBar->findChild<QWidget*>(qsl("qt_toolbar_ext_button")); }

    // The fit runs on the resize itself, with no timer behind it
    void resizeTo(const int width)
    {
        mpEditor->resize(width, scmSweepHeight);
        QCoreApplication::sendPostedEvents();
        QCoreApplication::processEvents();
    }

    int narrowestWindowWidth() const { return std::max(mpEditor->minimumWidth(), mpEditor->minimumSizeHint().width()); }

    // Read off the buttons rather than labelsShown, so a group the bookkeeping
    // thinks is named but whose buttons were never told fails
    bool groupIsNamed(const int index) const
    {
        const auto& group = mpEditor->mEditorToolBarGroups.at(index);
        for (QAction* pAction : group.actions) {
            auto* pButton = qobject_cast<QToolButton*>(mpEditor->toolBar->widgetForAction(pAction));
            if (!pButton || pButton->toolButtonStyle() != Qt::ToolButtonTextBesideIcon) {
                return false;
            }
        }
        return true;
    }

    QString stateAt(const int width) const
    {
        return qsl("%1px of window (%2px of bar): profile %3, item %4")
                .arg(QString::number(width),
                     QString::number(mpEditor->toolBar->width()),
                     groupIsNamed(scmProfileGroup) ? qsl("named") : qsl("pictures"),
                     groupIsNamed(scmItemGroup) ? qsl("named") : qsl("pictures"));
    }

    // The same figure the fit measures against the bar's own width
    int barLengthWanted() const { return mpEditor->toolBar->layout()->sizeHint().width(); }

    int windowChromeWidth() const { return mpEditor->width() - mpEditor->toolBar->width(); }

    // By hand: a resize to the width the window already has sends no event, so
    // the fit would not run on its own
    void undoTheForcing()
    {
        for (auto& group : mpEditor->mEditorToolBarGroups) {
            group.labelsShown = true;
        }
        mpEditor->applyEditorToolbarButtonStyles();
        mpEditor->fitEditorToolBarToItsLength();
        QTest::qWait(20ms);
    }

    // As a window width, which is what the sweeps compare it against
    int pictureBarLength()
    {
        resizeTo(scmSweepWidest);
        const int chrome = windowChromeWidth();
        for (auto& group : mpEditor->mEditorToolBarGroups) {
            group.labelsShown = false;
        }
        mpEditor->applyEditorToolbarButtonStyles();
        QTest::qWait(20ms);
        const int wanted = barLengthWanted() + chrome;
        undoTheForcing();
        return wanted;
    }

    // The item group's wording is the view's, so this differs in every view
    int itemGroupNamedLength()
    {
        resizeTo(scmSweepWidest);
        const int chrome = windowChromeWidth();
        mpEditor->mEditorToolBarGroups[scmProfileGroup].labelsShown = false;
        mpEditor->mEditorToolBarGroups[scmItemGroup].labelsShown = true;
        mpEditor->applyEditorToolbarButtonStyles();
        QTest::qWait(20ms);
        const int wanted = barLengthWanted() + chrome;
        undoTheForcing();
        return wanted;
    }

    struct Sweep
    {
        int profileGaveUp = -1;
        int bothGaveUp = -1;
        int folded = -1;
        int foldedWithANameStillWritten = -1;
        int narrowest = 0;
    };

    // Kept per icon size, the one thing the cases change that moves the breakpoints: five cases read the same sweep
    QHash<int, Sweep> mSweeps;

    Sweep sweepNarrowing()
    {
        const int preference = mudlet::self()->mToolbarIconSize;
        if (const auto known = mSweeps.constFind(preference); known != mSweeps.constEnd()) {
            return known.value();
        }
        Sweep result;
        resizeTo(scmSweepWidest);
        result.narrowest = narrowestWindowWidth();
        const QWidget* pExtension = extensionButton();
        int reachedBefore = 0;
        for (int width = scmSweepWidest; width >= result.narrowest; width -= scmSweepStep) {
            resizeTo(width);
            if (reachedBefore && mpEditor->width() >= reachedBefore) {
                break;
            }
            reachedBefore = mpEditor->width();
            const bool profileNamed = groupIsNamed(scmProfileGroup);
            const bool itemNamed = groupIsNamed(scmItemGroup);
            if (result.profileGaveUp < 0 && !profileNamed) {
                result.profileGaveUp = width;
            }
            if (result.bothGaveUp < 0 && !profileNamed && !itemNamed) {
                result.bothGaveUp = width;
            }
            if (pExtension && pExtension->isVisible()) {
                if (result.folded < 0) {
                    result.folded = width;
                }
                if (result.foldedWithANameStillWritten < 0 && (profileNamed || itemNamed)) {
                    result.foldedWithANameStillWritten = width;
                }
            }
        }
        mSweeps.insert(preference, result);
        return result;
    }

    // mudlet::setToolBarIconSize() drops a value equal to the one it holds, so
    // the step through a different value is what makes it run
    void setPreference(const int value)
    {
        mudlet::self()->setToolBarIconSize(value == 1 ? 2 : 1);
        mudlet::self()->setToolBarIconSize(value);
        QCoreApplication::sendPostedEvents();
        QTest::qWait(50ms);
    }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.start(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();

        mudlet::self()->slot_showEditorDialog();
        QTest::qWait(100ms);
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
        QVERIFY2(mpEditor->mEditorToolBarGroups.count() == 2, "The bar is expected to have a profile group and an item group");
        setPreference(scmDefaultPreference);
        mpEditor->slot_showTriggers();
        resizeTo(scmSweepWidest);
    }

    void cleanupTestCase()
    {
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    void test_everyGroupIsNamedOnAWideWindow()
    {
        resizeTo(scmSweepWidest);
        QVERIFY2(groupIsNamed(scmProfileGroup) && groupIsNamed(scmItemGroup), qPrintable(qsl("On a window with room to spare, %1").arg(stateAt(scmSweepWidest))));
    }

    void test_theProfileGroupIsTheFirstToStandAsPictures()
    {
        const Sweep sweep = sweepNarrowing();
        qInfo().noquote() << qsl("  narrowing from %1px to %2px: the profile group gives its names up at %3px, the item group at %4px")
                                     .arg(QString::number(scmSweepWidest), QString::number(sweep.narrowest), QString::number(sweep.profileGaveUp), QString::number(sweep.bothGaveUp));

        QVERIFY2(sweep.profileGaveUp > 0, "The profile group never gave its names up, so the bar was never asked to fit into anything");
        QVERIFY2(sweep.bothGaveUp < 0 || sweep.profileGaveUp > sweep.bothGaveUp,
                 qPrintable(qsl("The item group gave its names up at %1px and the profile group only at %2px - the wrong way round")
                                    .arg(QString::number(sweep.bothGaveUp), QString::number(sweep.profileGaveUp))));
    }

    void test_bothGroupsStandAsPicturesOnANarrowWindow()
    {
        const Sweep sweep = sweepNarrowing();
        QVERIFY2(sweep.bothGaveUp > 0,
                 qPrintable(qsl("Down to %1px, the narrowest the window goes, the item group never gave its names up: %2").arg(QString::number(sweep.narrowest), stateAt(mpEditor->width()))));
        resizeTo(sweep.narrowest);
        QVERIFY2(!groupIsNamed(scmProfileGroup) && !groupIsNamed(scmItemGroup), qPrintable(qsl("At the narrowest the window goes, %1").arg(stateAt(sweep.narrowest))));
    }

    void test_nothingIsFoldedAwayWhileANameIsStillWritten()
    {
        const Sweep sweep = sweepNarrowing();
        QVERIFY2(extensionButton() != nullptr, "The bar has no overflow button, so this case is watching nothing");
        qInfo().noquote() << qsl("  narrowing to %1px: the fold is first reached at %2px, by which width both groups are pictures")
                                     .arg(QString::number(sweep.narrowest), sweep.folded < 0 ? qsl("no width swept") : QString::number(sweep.folded));

        QVERIFY2(sweep.folded > 0,
                 qPrintable(qsl("the sweep down to %1px never reached a fold, so the reading below is of a bar that was never asked to post anything away and the case measured nothing")
                                    .arg(QString::number(sweep.narrowest))));
        QVERIFY2(sweep.foldedWithANameStillWritten < 0,
                 qPrintable(qsl("At %1px of window the bar posted actions into its overflow menu with names still written out on it - the fit had room left to make and did not take it. "
                                "Both groups only became pictures at %2px.")
                                    .arg(QString::number(sweep.foldedWithANameStillWritten), QString::number(sweep.bothGaveUp))));
    }

    void test_wideningGivesTheNamesBackInTheOtherOrder()
    {
        const Sweep sweep = sweepNarrowing();
        resizeTo(sweep.narrowest);
        QVERIFY2(!groupIsNamed(scmProfileGroup) && !groupIsNamed(scmItemGroup), qPrintable(qsl("This case starts from a bar of pictures, and instead %1").arg(stateAt(sweep.narrowest))));

        int itemNamedAt = -1;
        int profileNamedAt = -1;
        for (int width = sweep.narrowest; width <= scmSweepWidest; width += scmSweepStep) {
            resizeTo(width);
            if (itemNamedAt < 0 && groupIsNamed(scmItemGroup)) {
                itemNamedAt = width;
            }
            if (profileNamedAt < 0 && groupIsNamed(scmProfileGroup)) {
                profileNamedAt = width;
            }
        }
        qInfo().noquote() << qsl("  widening from %1px: the item group has its names back at %2px, the profile group at %3px")
                                     .arg(QString::number(sweep.narrowest), QString::number(itemNamedAt), QString::number(profileNamedAt));

        QVERIFY2(itemNamedAt > 0 && profileNamedAt > 0,
                 qPrintable(qsl("Widening back to %1px did not give both groups their names: item at %2, profile at %3")
                                    .arg(QString::number(scmSweepWidest), QString::number(itemNamedAt), QString::number(profileNamedAt))));
        QVERIFY2(itemNamedAt < profileNamedAt,
                 qPrintable(qsl("The profile group took its names back at %1px and the item group only at %2px - the last to give them up has to be the last to take them back")
                                    .arg(QString::number(profileNamedAt), QString::number(itemNamedAt))));
    }

    void test_aDragAcrossTheBreakpointDoesNotFlicker()
    {
        const Sweep sweep = sweepNarrowing();
        QVERIFY2(sweep.profileGaveUp > 0, "No breakpoint was found to drag across");

        resizeTo(sweep.profileGaveUp);
        const bool settled = groupIsNamed(scmProfileGroup);
        QStringList seen;
        for (int pass = 0; pass < 10; ++pass) {
            resizeTo(sweep.profileGaveUp + 2);
            const bool wider = groupIsNamed(scmProfileGroup);
            resizeTo(sweep.profileGaveUp);
            const bool narrower = groupIsNamed(scmProfileGroup);
            if (wider != settled || narrower != settled) {
                seen.append(qsl("pass %1: %2px %3, %4px %5")
                                    .arg(QString::number(pass),
                                         QString::number(sweep.profileGaveUp + 2),
                                         wider ? qsl("named") : qsl("pictures"),
                                         QString::number(sweep.profileGaveUp),
                                         narrower ? qsl("named") : qsl("pictures")));
            }
        }
        QVERIFY2(seen.isEmpty(),
                 qPrintable(qsl("Two pixels either side of the %1px breakpoint changed the profile group, so a drag over it flickers:\n  %2")
                                    .arg(QString::number(sweep.profileGaveUp), seen.join(qsl("\n  ")))));

        // Reported to judge whether a burst of resizes needs coalescing behind a zero-timer
        QElapsedTimer timer;
        timer.start();
        constexpr int runs = 1000;
        for (int run = 0; run < runs; ++run) {
            mpEditor->fitEditorToolBarToItsLength();
        }
        qInfo().noquote() << qsl("  the fit costs %1us a call at a width it settles at").arg(QString::number(static_cast<double>(timer.nsecsElapsed()) / (runs * 1000.0), 'f', 1));
    }

    void test_switchingViewRemeasuresForTheNewWording()
    {
        mpEditor->slot_showTriggers();
        const int triggerViewBoundary = itemGroupNamedLength();
        mpEditor->slot_showAliases();
        const int aliasViewBoundary = itemGroupNamedLength();
        qInfo().noquote() << qsl("  with the profile group already pictures, the bar wants %1px of window for the trigger view's names and %2px for the alias view's")
                                     .arg(QString::number(triggerViewBoundary), QString::number(aliasViewBoundary));

        QVERIFY2(aliasViewBoundary < triggerViewBoundary,
                 qPrintable(qsl("The two views' wordings measure the same (%1px against %2px), so nothing here would show a re-measurement")
                                    .arg(QString::number(aliasViewBoundary), QString::number(triggerViewBoundary))));

        // +20 clears the margin names are taken back with, so only the trigger
        // wording is too wide here
        const int between = aliasViewBoundary + 20;
        QVERIFY2(between < triggerViewBoundary,
                 qPrintable(qsl("The wordings are %1px apart, which leaves no width that is roomy for one and not for the other").arg(QString::number(triggerViewBoundary - aliasViewBoundary))));

        resizeTo(scmSweepWidest);
        mpEditor->slot_showTriggers();
        QTest::qWait(20ms);
        resizeTo(between);
        const bool namedInTriggerView = groupIsNamed(scmItemGroup);
        mpEditor->slot_showAliases();
        QTest::qWait(20ms);
        const bool namedAfterSwitching = groupIsNamed(scmItemGroup);

        resizeTo(scmSweepWidest);
        QTest::qWait(20ms);
        resizeTo(between);
        const bool namedAfterResizing = groupIsNamed(scmItemGroup);

        qInfo().noquote() << qsl("  at %1px of window the item group is %2 in the trigger view, %3 after switching to the alias view, and %4 on a fresh resize there")
                                     .arg(QString::number(between),
                                          namedInTriggerView ? qsl("named") : qsl("pictures"),
                                          namedAfterSwitching ? qsl("named") : qsl("pictures"),
                                          namedAfterResizing ? qsl("named") : qsl("pictures"));

        QVERIFY2(!namedInTriggerView, "The trigger view's wording was supposed to be too wide for this window, and the item group kept its names");
        QVERIFY2(namedAfterSwitching == namedAfterResizing,
                 qPrintable(qsl("Switching to the alias view left the item group %1, where a resize to the same width in that view leaves it %2 - the view change did not re-measure")
                                    .arg(namedAfterSwitching ? qsl("named") : qsl("pictures"), namedAfterResizing ? qsl("named") : qsl("pictures"))));
        QVERIFY2(namedAfterSwitching, "The alias view's wording fits this window with room to spare, so the item group should have taken its names back");
        mpEditor->slot_showTriggers();
    }

    void test_everyCollapsingButtonIsNamedByItsTooltip()
    {
        resizeTo(scmSweepWidest);
        QStringList silent;
        for (const auto& group : mpEditor->mEditorToolBarGroups) {
            for (const QAction* pAction : group.actions) {
                const QString wording = pAction->text();
                const QString tip = pAction->toolTip();
                if (wording.isEmpty() || !tip.contains(wording)) {
                    silent.append(qsl("%1 -> \"%2\"").arg(wording.isEmpty() ? qsl("<unnamed action>") : wording, tip));
                }
            }
        }
        QVERIFY2(silent.isEmpty(), qPrintable(qsl("These buttons do not say their own name in their tooltip, so they say nothing once they are pictures:\n  %1").arg(silent.join(qsl("\n  ")))));
    }

    // A bar of pictures is wider than the narrowest window, so Qt's fold stays
    // reachable. docs/design-language.md quotes the numbers this reports.
    void test_theBarOfPicturesIsMeasuredAgainstTheNarrowestWindow()
    {
        QStringList measured;
        for (const int preference : {scmDefaultPreference, scmLargestPreference}) {
            setPreference(preference);
            const int pictureBar = pictureBarLength();
            const Sweep sweep = sweepNarrowing();
            measured.append(qsl("at preference %1 (%2px glyphs) the profile group gives its names up at %3px and the item group at %4px; a bar of pictures wants %5px, the window goes down to %6px, "
                                "and the fold is reached at %7px")
                                    .arg(QString::number(preference),
                                         QString::number(mpEditor->toolBar->iconSize().width()),
                                         QString::number(sweep.profileGaveUp),
                                         QString::number(sweep.bothGaveUp),
                                         QString::number(pictureBar),
                                         QString::number(sweep.narrowest),
                                         sweep.folded < 0 ? qsl("no swept width") : QString::number(sweep.folded)));

            QVERIFY2(sweep.folded > 0, qPrintable(qsl("the sweep never reached a fold, so the case measured nothing: %1").arg(measured.constLast())));
            QVERIFY2(sweep.folded <= pictureBar, qPrintable(measured.constLast()));
            QVERIFY2(sweep.bothGaveUp > 0 && sweep.bothGaveUp > sweep.folded, qPrintable(measured.constLast()));
        }
        qInfo().noquote() << qsl("  %1").arg(measured.join(qsl("\n  ")));
        setPreference(scmDefaultPreference);
    }

    // The bar's width comes down to its pictures here, and the extent only holds the length of the button
    void test_aBarDownTheWindowsEdgeKeepsTheWholeWord()
    {
        auto* pExtension = qobject_cast<QToolButton*>(extensionButton());
        QVERIFY2(pExtension != nullptr, "The bar has no overflow button to read");
        const QSize was = mpEditor->size();
        const auto restore = qScopeGuard([this, was]() {
            static_cast<QMainWindow*>(mpEditor)->addToolBar(Qt::TopToolBarArea, mpEditor->toolBar);
            mpEditor->resize(was);
            QTest::qWait(50ms);
        });
        static_cast<QMainWindow*>(mpEditor)->addToolBar(Qt::LeftToolBarArea, mpEditor->toolBar);
        mpEditor->resize(scmSweepWidest, mpEditor->minimumHeight());
        QTest::qWait(150ms);
        QVERIFY2(pExtension->isVisible(), "Nothing folded in the shortest window, so the width below proves nothing");

        QCOMPARE(pExtension->toolButtonStyle(), Qt::ToolButtonTextUnderIcon);
        const QSize wanted = pExtension->QToolButton::sizeHint();
        QVERIFY2(pExtension->width() >= wanted.width() && pExtension->height() >= wanted.height(),
                 qPrintable(qsl("%1x%2 is too small for \"%3\", which needs %4x%5")
                                    .arg(QString::number(pExtension->width()),
                                         QString::number(pExtension->height()),
                                         pExtension->text(),
                                         QString::number(wanted.width()),
                                         QString::number(wanted.height()))));
    }

    void test_theFoldsOwnButtonShowsHowManyAreHidden()
    {
        auto* pExtension = qobject_cast<QToolButton*>(extensionButton());
        QVERIFY2(pExtension != nullptr, "The bar has no overflow button to read");
        const auto restore = qScopeGuard([this]() {
            resizeTo(scmSweepWidest);
        });
        int width = pictureBarLength() - 2 * scmSweepStep;
        resizeTo(width);
        for (; !pExtension->isVisible() && width > narrowestWindowWidth(); width -= scmSweepStep) {
            resizeTo(width);
        }
        QVERIFY2(pExtension->isVisible(), "Nothing folded at the narrowest window, so the count below proves nothing");

        // In one step, as a window snapped narrow does: the bar folds and names go in the same layout pass
        resizeTo(scmSweepWidest);
        resizeTo(narrowestWindowWidth());
        QTest::qWait(50ms);

        int hidden = 0;
        for (QAction* pAction : mpEditor->toolBar->actions()) {
            const QWidget* pWidget = mpEditor->toolBar->widgetForAction(pAction);
            if (pWidget && pAction->isVisible() && !pAction->isSeparator() && pWidget->metaObject() != &QWidget::staticMetaObject && pWidget->isHidden()) {
                ++hidden;
            }
        }
        QVERIFY2(hidden > 0, "The overflow button shows with no button hidden");
        QCOMPARE(pExtension->text(), uiDesign::ToolBarOverflow::word(hidden));
        QCOMPARE(pExtension->width(), mpEditor->toolBar->property(uiDesign::scmProp_toolBarOverflowExtent).toInt());
    }

    void test_theFoldsOwnButtonIsTheDesignsChevronAndCount()
    {
        auto* pExtension = qobject_cast<QToolButton*>(extensionButton());
        QVERIFY2(pExtension != nullptr, "The bar has no overflow button to draw");
        const QIcon chevron = pExtension->icon();
        QVERIFY2(!chevron.isNull(), "The overflow button carries no picture of ours, so the style's own is what shows");

        const QImage drawn = chevron.pixmap(pExtension->iconSize()).toImage();
        QVERIFY2(!drawn.isNull(), "The overflow button's picture is empty");
        QColor strongest;
        int strongestAlpha = 0;
        for (int x = 0; x < drawn.width(); ++x) {
            for (int y = 0; y < drawn.height(); ++y) {
                const QColor pixel = drawn.pixelColor(x, y);
                if (pixel.alpha() > strongestAlpha) {
                    strongestAlpha = pixel.alpha();
                    strongest = pixel;
                }
            }
        }
        const QColor lit = uiDesign::themeTokens().mutedText;
        qInfo().noquote() << qsl("  the chevron's strongest pixel is %1 at alpha %2, against the chrome tone %3").arg(strongest.name(), QString::number(strongestAlpha), lit.name());
        QVERIFY2(strongestAlpha > 200, qPrintable(qsl("The chevron is barely drawn - its strongest pixel is alpha %1").arg(QString::number(strongestAlpha))));
        // Tinting goes through premultiplied alpha, which can land a level off
        const bool sameInk = std::abs(strongest.red() - lit.red()) <= 2 && std::abs(strongest.green() - lit.green()) <= 2 && std::abs(strongest.blue() - lit.blue()) <= 2;
        QVERIFY2(sameInk, qPrintable(qsl("The chevron is inked %1 rather than the chrome tone %2").arg(strongest.name(), lit.name())));
    }
};

#include "EditorToolBarOverflowTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorToolBarOverflowTest)
