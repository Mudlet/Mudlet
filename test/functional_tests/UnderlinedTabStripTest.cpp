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
 * Every strip drawn by uiDesign::tabBarStyleSheet() and prepareTabStrip() - the
 * notepad's notes, the package manager's views and both of the connection
 * window's strips - is read off a grab in both appearances: the chosen tab's
 * word stands on an accent underline and no other tab's does, the first word
 * starts on the line the content under the strip starts on, choosing another
 * tab moves nothing, no word is cut short, and the hairline the tabs stand on
 * runs on past the last tab to the end of what is under the strip.
 *
 * Run with: ctest -R UnderlinedTabStripTest -V
 */

#include <QFontMetricsF>
#include <QHash>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPlainTextEdit>
#include <QTabBar>
#include <QTabWidget>
#include <QToolButton>
#include <QtTest/QtTest>

#include <functional>

#include "Host.h"
#include "HostDialogs.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "dlgConnectionProfiles.h"
#include "dlgNotepad.h"
#include "dlgPackageManager.h"
#include "mudlet.h"
#include "uiDesign.h"

#include "GroupedTest.h"

class UnderlinedTabStripTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    Host* mpHost = nullptr;
    dlgNotepad* mpNotepad = nullptr;
    dlgPackageManager* mpPackages = nullptr;
    dlgConnectionProfiles* mpConnection = nullptr;
    const QString mProfileName = qsl("UnderlinedTabStrip-Test-Profile");

    // A flat fill or a hairline is written, not blended, so it reads back to a level or two
    static constexpr int scmInkTolerance = 2;
    // A glyph's ink starts a fraction of a pixel off the side bearing its font reports
    static constexpr int scmBearingTolerance = 1;
    // The halfway threshold can leave out the faintest column at either end of a word's ink
    static constexpr int scmInkExtentSlack = 2;

    struct Strip
    {
        QString what;
        QTabBar* pBar = nullptr;
        // Grabbed whole: a see-through bar grabbed on its own comes back without what shows through it
        QWidget* pWindow = nullptr;
        // In pWindow's coordinates
        int contentLeft = 0;
        int contentRight = 0;
    };

    static QRect inWindow(const Strip& strip, const QRect& inBar) { return QRect(strip.pBar->mapTo(strip.pWindow, inBar.topLeft()), inBar.size()); }

    // The row the tabs stand on, which the chosen tab's underline is drawn over
    static int hairlineRow(const Strip& strip) { return strip.pBar->mapTo(strip.pWindow, QPoint(0, strip.pBar->height() - 1)).y(); }

    static QColor commonestColour(const QImage& grab, const QRect& logical)
    {
        QHash<QRgb, int> counts;
        for (int y = logical.top(); y <= logical.bottom(); ++y) {
            for (int x = logical.left(); x <= logical.right(); ++x) {
                const QColor read = TestPixels::sampled(grab, QPoint(x, y));
                if (read.isValid()) {
                    ++counts[read.rgb()];
                }
            }
        }
        QRgb commonest = 0;
        int most = -1;
        for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
            if (it.value() > most) {
                most = it.value();
                commonest = it.key();
            }
        }
        return QColor(commonest);
    }

    // The line the word is laid in: the tab less its trailing gap, the air above it and the underline's room below
    static QRect wordLine(const Strip& strip, const int index)
    {
        const QRect tab = strip.pBar->tabRect(index);
        return inWindow(strip, QRect(tab.left(), tab.top() + uiDesign::scmTabPaddingTop, tab.width() - uiDesign::scmTabGap, tab.height() - uiDesign::scmTabPaddingTop - uiDesign::scmTabPaddingBottom));
    }

    // First and last column of the word's ink, thresholded halfway to the word's own ink so antialiased fringes count
    // only where they are more word than surface. The cross, if any, is left out.
    static QPair<int, int> inkSpan(const QImage& grab, const Strip& strip, const int index, const QColor& ink)
    {
        QRect line = wordLine(strip, index);
        if (QWidget* pCross = strip.pBar->tabButton(index, QTabBar::RightSide)) {
            line.setRight(strip.pBar->mapTo(strip.pWindow, pCross->geometry().topLeft()).x() - 1);
        }
        const QColor behind = commonestColour(grab, line);
        const int reach = TestPixels::distance(behind, ink);
        int first = -1;
        int last = -1;
        for (int x = line.left(); x <= line.right(); ++x) {
            for (int y = line.top(); y <= line.bottom(); ++y) {
                if (2 * TestPixels::distance(TestPixels::sampled(grab, QPoint(x, y)), behind) >= reach) {
                    first = first < 0 ? x : first;
                    last = x;
                    break;
                }
            }
        }
        return {first, last};
    }

    // Device pixels past halfway from the surface to the ink, the threshold inkSpan() reads a column by
    static int inkCoverage(const QImage& grab, const QRect& logical, const QColor& behind, const QColor& ink)
    {
        const qreal ratio = grab.devicePixelRatio();
        const QRect scanned = QRect(qRound(logical.left() * ratio), qRound(logical.top() * ratio), qRound(logical.width() * ratio), qRound(logical.height() * ratio)) & grab.rect();
        const int reach = TestPixels::distance(behind, ink);
        int covered = 0;
        for (int y = scanned.top(); y <= scanned.bottom(); ++y) {
            for (int x = scanned.left(); x <= scanned.right(); ++x) {
                covered += 2 * TestPixels::distance(grab.pixelColor(x, y), behind) >= reach ? 1 : 0;
            }
        }
        return covered;
    }

    // The same word drawn here at one weight, on the same surface in the same ink, for the grab's to be held against
    static int coverageAt(const QFont& font, const QString& text, const QColor& ink, const QColor& behind, const qreal ratio)
    {
        const QFontMetrics metrics(font);
        QImage drawn(QSize(metrics.horizontalAdvance(text) + 8, metrics.height() + 4) * ratio, QImage::Format_ARGB32);
        drawn.setDevicePixelRatio(ratio);
        drawn.fill(behind);
        QPainter painter(&drawn);
        painter.setFont(font);
        painter.setPen(ink);
        painter.drawText(QPoint(4, 2 + metrics.ascent()), text);
        painter.end();
        return inkCoverage(drawn, QRect(QPoint(0, 0), drawn.deviceIndependentSize().toSize()), behind, ink);
    }

    static QFont drawnFont(const QTabBar* pBar, const int index)
    {
        QFont font = pBar->font();
        if (index == pBar->currentIndex()) {
            font.setWeight(uiDesign::scmTabChosenWeight);
        }
        return font;
    }

    static QColor wordInk(const QTabBar* pBar, const int index, const uiDesign::ThemeTokens& tokens) { return index == pBar->currentIndex() ? tokens.accentText : tokens.mutedText; }

    void readStrip(const Strip& strip, const QString& appearance)
    {
        QTabBar* pBar = strip.pBar;
        QVERIFY2(pBar && pBar->isVisible(), qPrintable(qsl("%1: the strip is not on show").arg(strip.what)));
        QVERIFY2(pBar->count() >= 2, qPrintable(qsl("SETUP: %1 holds %2 tabs, so there is no unchosen one to read").arg(strip.what, QString::number(pBar->count()))));
        pBar->setCurrentIndex(0);
        TestPixels::settle();

        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QString where = qsl("%1, %2 appearance").arg(strip.what, appearance);
        QImage grab = TestPixels::shot(strip.pWindow);
        const int underlineRow = hairlineRow(strip);

        // (a) the chosen word stands on the accent; nothing else does
        const auto [chosenFirst, chosenLast] = inkSpan(grab, strip, 0, wordInk(pBar, 0, tokens));
        QVERIFY2(chosenFirst >= 0, qPrintable(qsl("%1: the chosen tab's word left no ink to read").arg(where)));
        for (int row = underlineRow - uiDesign::scmTabUnderlineWidth + 1; row <= underlineRow; ++row) {
            for (int x = chosenFirst; x <= chosenLast; ++x) {
                const QColor read = TestPixels::sampled(grab, QPoint(x, row));
                QVERIFY2(TestPixels::readsAs(read, tokens.accent, scmInkTolerance),
                         qPrintable(qsl("%1: under the chosen word, at %2,%3, the strip reads %4 rather than the accent %5 - the chosen tab stands on no underline")
                                            .arg(where, QString::number(x), QString::number(row), read.name(), tokens.accent.name())));
            }
        }
        for (int index = 1; index < pBar->count(); ++index) {
            const QRect tab = inWindow(strip, pBar->tabRect(index));
            for (int row = underlineRow - uiDesign::scmTabUnderlineWidth + 1; row <= underlineRow; ++row) {
                for (int x = tab.left(); x <= tab.right(); ++x) {
                    const QColor read = TestPixels::sampled(grab, QPoint(x, row));
                    QVERIFY2(!TestPixels::readsAs(read, tokens.accent, scmInkTolerance),
                             qPrintable(qsl("%1: tab %2, which is not chosen, carries the accent at %3,%4 - it reads as chosen too")
                                                .arg(where, QString::number(index), QString::number(x), QString::number(row))));
                }
            }
        }

        // (b) the first word starts on the content's line
        const QString firstWord = pBar->tabText(0);
        const int bearing = std::max(0, qRound(QFontMetricsF(drawnFont(pBar, 0)).leftBearing(firstWord.at(0))));
        qInfo().noquote() << qsl("  %1: the first word's ink starts at %2 with a %3px bearing, the content under the strip at %4")
                                     .arg(where, QString::number(chosenFirst), QString::number(bearing), QString::number(strip.contentLeft));
        QVERIFY2(std::abs(chosenFirst - bearing - strip.contentLeft) <= scmBearingTolerance,
                 qPrintable(qsl("%1: the first word's ink starts at %2 (a %3px bearing in), off the line the content under the strip starts on, %4")
                                    .arg(where, QString::number(chosenFirst), QString::number(bearing), QString::number(strip.contentLeft))));

        // No word is cut short, chosen or not: its ink is as wide as the word's own ink drawn whole. And each is drawn at
        // its own weight: its ink covers nearer what the word covers drawn at that weight than at the other
        QFont chosenWeight = pBar->font();
        chosenWeight.setWeight(uiDesign::scmTabChosenWeight);
        for (int index = 0; index < pBar->count(); ++index) {
            const QColor ink = wordInk(pBar, index, tokens);
            const auto [first, last] = inkSpan(grab, strip, index, ink);
            const int inked = first < 0 ? 0 : last - first + 1;
            const int whole = qRound(QFontMetricsF(drawnFont(pBar, index)).tightBoundingRect(pBar->tabText(index)).width());
            QVERIFY2(first >= 0 && inked + scmInkExtentSlack >= whole,
                     qPrintable(qsl("%1: tab %2's word inks %3px of the %4px \"%5\" inks drawn whole, so it is cut short")
                                        .arg(where, QString::number(index), QString::number(inked), QString::number(whole), pBar->tabText(index))));

            QRect line = wordLine(strip, index);
            line.setLeft(first);
            line.setRight(last);
            const QColor behind = commonestColour(grab, wordLine(strip, index));
            const int covered = inkCoverage(grab, line, behind, ink);
            const int atBarWeight = coverageAt(pBar->font(), pBar->tabText(index), ink, behind, grab.devicePixelRatio());
            const int atChosenWeight = coverageAt(chosenWeight, pBar->tabText(index), ink, behind, grab.devicePixelRatio());
            QVERIFY2(atChosenWeight > atBarWeight,
                     qPrintable(qsl("SETUP: \"%1\" covers no more drawn at the chosen weight than at the bar's, so its weight cannot be read").arg(pBar->tabText(index))));
            const bool readsChosen = std::abs(covered - atChosenWeight) < std::abs(covered - atBarWeight);
            QVERIFY2(readsChosen == (index == pBar->currentIndex()),
                     qPrintable(qsl("%1: tab %2's word covers %3 pixels, where \"%4\" covers %5 at the bar's weight and %6 at the chosen one - it is drawn at the %7 weight")
                                        .arg(where,
                                             QString::number(index),
                                             QString::number(covered),
                                             pBar->tabText(index),
                                             QString::number(atBarWeight),
                                             QString::number(atChosenWeight),
                                             readsChosen ? qsl("chosen") : qsl("bar's"))));
        }

        // (d) the hairline runs on past the last tab to the end of the content under the strip
        const int lastTabEnd = inWindow(strip, pBar->tabRect(pBar->count() - 1)).right();
        QVERIFY2(strip.contentRight > lastTabEnd, qPrintable(qsl("SETUP: %1's tabs reach %2, the end of the content, so nothing beyond them could be read").arg(where, QString::number(lastTabEnd))));
        const QColor lineEnd = TestPixels::sampled(grab, QPoint(strip.contentRight, underlineRow));
        QVERIFY2(TestPixels::readsAs(lineEnd, tokens.border, scmInkTolerance),
                 qPrintable(qsl("%1: at the end of the content, %2,%3, the strip's bottom row reads %4 rather than the hairline %5")
                                    .arg(where, QString::number(strip.contentRight), QString::number(underlineRow), lineEnd.name(), tokens.border.name())));
        const QColor overTheLine = TestPixels::sampled(grab, QPoint(strip.contentRight, underlineRow - 1));
        QVERIFY2(!TestPixels::readsAs(overTheLine, tokens.border, scmInkTolerance), qPrintable(qsl("%1: the row over the hairline is the hairline's colour too, so it is no hairline").arg(where)));

        // (c) choosing another tab moves nothing
        const QSize stripSize = pBar->size();
        QList<QRect> before;
        for (int index = 0; index < pBar->count(); ++index) {
            before << pBar->tabRect(index);
        }
        pBar->setCurrentIndex(1);
        TestPixels::settle();
        QVERIFY2(pBar->size() == stripSize,
                 qPrintable(qsl("%1: the strip was %2 and is %3 once another tab is chosen").arg(where, TestPixels::describe(stripSize), TestPixels::describe(pBar->size()))));
        for (int index = 0; index < pBar->count(); ++index) {
            QVERIFY2(pBar->tabRect(index) == before.at(index),
                     qPrintable(qsl("%1: tab %2 stood at %3 and moved to %4 when the choice moved, so the strip steps sideways for the semibold word")
                                        .arg(where, QString::number(index), TestPixels::describe(before.at(index)), TestPixels::describe(pBar->tabRect(index)))));
        }
        grab = TestPixels::shot(strip.pWindow);
        const auto [movedFirst, movedLast] = inkSpan(grab, strip, 1, wordInk(pBar, 1, tokens));
        QVERIFY2(movedFirst >= 0, qPrintable(qsl("%1: the newly chosen tab's word left no ink to read").arg(where)));
        const QColor underMoved = TestPixels::sampled(grab, QPoint((movedFirst + movedLast) / 2, underlineRow));
        QVERIFY2(TestPixels::readsAs(underMoved, tokens.accent, scmInkTolerance), qPrintable(qsl("%1: the underline did not follow the choice to tab 1: it reads %2").arg(where, underMoved.name())));
        pBar->setCurrentIndex(0);
        TestPixels::settle();
    }

    void readInBothAppearances(const std::function<Strip()>& stripNow)
    {
        for (const auto& appearance : QList<QPair<QString, enums::Appearance>>{{qsl("dark"), enums::Appearance::dark}, {qsl("light"), enums::Appearance::light}}) {
            const Strip strip = stripNow();
            TestPixels::setAppearanceAndWait(appearance.second, strip.pWindow);
            // Measured again: a restyle can move the content under the strip
            readStrip(stripNow(), appearance.first);
            if (QTest::currentTestFailed()) {
                return;
            }
        }
    }

    static int leftIn(const QWidget* pWidget, const QWidget* pWindow) { return pWidget->mapTo(pWindow, QPoint(0, 0)).x(); }
    static int rightIn(const QWidget* pWidget, const QWidget* pWindow) { return pWidget->mapTo(pWindow, QPoint(pWidget->width() - 1, 0)).x(); }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.startMudlet(), qPrintable(mSession.why()));
        mudlet::self()->setInterfaceLanguage(qsl("en_US"));
        QVERIFY2(mSession.openProfile(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();

        mudlet::self()->slot_notes();
        mpNotepad = HostDialogs::of(mpHost).mpNotePad;
        QVERIFY2(mpNotepad, "the notepad did not open at all");
        mpNotepad->resize(760, 520);
        mpNotepad->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpNotepad));
        if (mpNotepad->tabWidget->count() < 2) {
            auto* pAdd = mpNotepad->findChild<QToolButton*>(qsl("notepadAddTab"));
            QVERIFY2(pAdd, "the notepad has no 'notepadAddTab' to make a second note with");
            pAdd->click();
        }
        QCOMPARE(mpNotepad->tabWidget->count(), 2);

        mpPackages = new dlgPackageManager(nullptr, mpHost);
        mpPackages->resize(900, 600);
        mpPackages->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpPackages));

        // A profile is chosen, so the form rather than the first-launch invitation is on show
        mpConnection = new dlgConnectionProfiles(mudlet::self());
        mpConnection->fillout_form();
        const auto items = mpConnection->findData(*mpConnection->listWidget_profiles, mProfileName, dlgConnectionProfiles::csmNameRole);
        QVERIFY2(!items.isEmpty(), "the profile is not in the connection window's games list");
        mpConnection->listWidget_profiles->setCurrentItem(items.first());
        mpConnection->resize(880, 560);
        mpConnection->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpConnection));
        TestPixels::settle();
    }

    void cleanupTestCase()
    {
        if (mpConnection) {
            mpConnection->close();
            delete mpConnection;
            mpConnection = nullptr;
        }
        if (mpPackages) {
            mpPackages->close();
            delete mpPackages;
            mpPackages = nullptr;
        }
        mpNotepad = nullptr;
        mpHost = nullptr;
        if (mudlet::self()) {
            mudlet::self()->setAppearance(enums::Appearance::systemSetting);
        }
        mSession.stop();
    }

    void test_theNotepadsNotes()
    {
        readInBothAppearances([this]() {
            auto* pNote = qobject_cast<QPlainTextEdit*>(mpNotepad->tabWidget->currentWidget());
            return Strip{qsl("the notepad"), mpNotepad->tabWidget->tabBar(), mpNotepad, leftIn(pNote, mpNotepad), rightIn(pNote, mpNotepad)};
        });
        if (QTest::currentTestFailed()) {
            return;
        }

        // The + stands on the same line, at its far end
        auto* pAdd = mpNotepad->findChild<QToolButton*>(qsl("notepadAddTab"));
        QVERIFY(pAdd);
        const Strip strip{qsl("the notepad"), mpNotepad->tabWidget->tabBar(), mpNotepad, 0, 0};
        auto* pNote = mpNotepad->tabWidget->currentWidget();
        const QRect add(pAdd->mapTo(mpNotepad, QPoint(0, 0)), pAdd->size());
        qInfo().noquote() << qsl("  the + stands at %1 over the hairline on row %2, the note ends at %3")
                                     .arg(TestPixels::describe(add), QString::number(hairlineRow(strip)), QString::number(rightIn(pNote, mpNotepad)));
        QVERIFY2(add.bottom() + 1 == hairlineRow(strip),
                 qPrintable(qsl("the + ends on row %1 rather than standing on the hairline on row %2").arg(QString::number(add.bottom()), QString::number(hairlineRow(strip)))));
        // Its sheet's margin is the box it draws in, which is what has to end with the line
        const int boxRight = add.right() - uiDesign::scmTabPaneInset;
        QVERIFY2(boxRight == rightIn(pNote, mpNotepad),
                 qPrintable(qsl("the +'s box ends at %1 rather than where the hairline and the note do, %2").arg(QString::number(boxRight), QString::number(rightIn(pNote, mpNotepad)))));

        // Under the pointer the box is washed, which shows where the margin left it; a stylesheet's :hover is read
        // off WA_UnderMouse, which no pointer sets offscreen
        pAdd->setAttribute(Qt::WA_UnderMouse, true);
        const QImage grab = TestPixels::shot(mpNotepad);
        const QColor outside = TestPixels::sampled(grab, QPoint(add.right() - 1, add.center().y()));
        const QColor inside = TestPixels::sampled(grab, QPoint(boxRight - 1, add.center().y()));
        pAdd->setAttribute(Qt::WA_UnderMouse, false);
        qInfo().noquote() << qsl("  under the pointer the + reads %1 inside its box and %2 in its margin").arg(inside.name(), outside.name());
        QVERIFY2(inside != outside, qPrintable(qsl("the +'s hover wash reaches into its margin (%1 both inside and out), so the box is not where its sheet says").arg(inside.name())));
    }

    // From the keyboard the chosen tab's underline thickens up into the air under the word, and nothing else moves;
    // a click gives the bar no focus, as it takes focus by Tab alone
    void test_theKeyboardThickensTheUnderline()
    {
        QTabBar* pBar = mpNotepad->tabWidget->tabBar();
        const Strip strip{qsl("the notepad"), pBar, mpNotepad, 0, 0};
        QVERIFY2(pBar->focusPolicy() == Qt::TabFocus, "the notepad's strip takes the focus some other way than by Tab, so a click could leave the thick underline behind");
        pBar->setCurrentIndex(0);
        mpNotepad->activateWindow();
        if (!QTest::qWaitForWindowActive(mpNotepad)) {
            QSKIP("the notepad cannot be made the active window here, so no widget in it can hold the focus");
        }
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const int underlineRow = hairlineRow(strip);
        mpNotepad->tabWidget->currentWidget()->setFocus(Qt::OtherFocusReason);
        TestPixels::settle();
        QVERIFY2(!pBar->hasFocus(), "SETUP: the strip still holds the focus with the note given it");
        const QImage resting = TestPixels::shot(mpNotepad);
        const auto [restingFirst, restingLast] = inkSpan(resting, strip, 0, tokens.accentText);
        const QRect restingTab = pBar->tabRect(0);

        pBar->setFocus(Qt::TabFocusReason);
        TestPixels::settle();
        QVERIFY2(pBar->hasFocus(), "SETUP: the strip did not take the focus it was given");
        const QImage focused = TestPixels::shot(mpNotepad);
        const auto [focusedFirst, focusedLast] = inkSpan(focused, strip, 0, tokens.accentText);
        QVERIFY2(focusedFirst == restingFirst && focusedLast == restingLast && pBar->tabRect(0) == restingTab, "the chosen word or its tab moved when the strip took the focus");
        for (int row = underlineRow - uiDesign::scmTabFocusUnderlineWidth + 1; row <= underlineRow; ++row) {
            const QColor read = TestPixels::sampled(focused, QPoint((focusedFirst + focusedLast) / 2, row));
            QVERIFY2(TestPixels::readsAs(read, tokens.accent, scmInkTolerance),
                     qPrintable(qsl("from the keyboard the underline is not %1px deep: row %2 reads %3").arg(QString::number(uiDesign::scmTabFocusUnderlineWidth), QString::number(row), read.name())));
        }
        const QColor overIt = TestPixels::sampled(focused, QPoint((focusedFirst + focusedLast) / 2, underlineRow - uiDesign::scmTabFocusUnderlineWidth));
        QVERIFY2(!TestPixels::readsAs(overIt, tokens.accent, scmInkTolerance), "from the keyboard the underline grows deeper than the design's four pixels");
        const QColor restingRow = TestPixels::sampled(resting, QPoint((restingFirst + restingLast) / 2, underlineRow - uiDesign::scmTabUnderlineWidth));
        QVERIFY2(!TestPixels::readsAs(restingRow, tokens.accent, scmInkTolerance), "the underline is already thicker than two pixels with no focus on the strip");

        pBar->clearFocus();
        QTest::mouseClick(pBar, Qt::LeftButton, Qt::NoModifier, pBar->tabRect(1).center());
        TestPixels::settle();
        QVERIFY2(pBar->currentIndex() == 1 && !pBar->hasFocus(), "a click on a tab chose it and left the strip holding the focus");
        pBar->setCurrentIndex(0);
        TestPixels::settle();
    }

    void test_thePackageManagersViews()
    {
        readInBothAppearances([this]() {
            return Strip{qsl("the package manager"),
                         mpPackages->findChild<QTabBar*>(qsl("packagesViewBar")),
                         mpPackages,
                         leftIn(mpPackages->packageList, mpPackages),
                         rightIn(mpPackages->packageList, mpPackages)};
        });
        QVERIFY2(leftIn(mpPackages->lineEdit_searchBar, mpPackages) == leftIn(mpPackages->packageList, mpPackages), "the search field and the list under the strip do not start on one line");
    }

    void test_theConnectionWindowsGames()
    {
        readInBothAppearances([this]() {
            return Strip{qsl("the connection window's games"),
                         mpConnection->findChild<QTabBar*>(qsl("gamesTabBar")),
                         mpConnection,
                         leftIn(mpConnection->listWidget_profiles, mpConnection),
                         rightIn(mpConnection->listWidget_profiles, mpConnection)};
        });
    }

    void test_theConnectionWindowsForm()
    {
        readInBothAppearances([this]() {
            QTabWidget* pTabs = mpConnection->tabWidget_connectionInfo;
            return Strip{qsl("the connection window's form"),
                         pTabs->tabBar(),
                         mpConnection,
                         leftIn(mpConnection->label_profileName, mpConnection),
                         rightIn(mpConnection->tab_connection_info, mpConnection)};
        });
    }
};

#include "UnderlinedTabStripTest.moc"
MUDLET_GROUPED_TEST_MAIN(UnderlinedTabStripTest)
