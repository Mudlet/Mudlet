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
 * The debug filter bar's glyphs are inked from the palette and retinted when it moves. Pause
 * carries both states in its QIcon's On slot, which one file for both would hide, and Show is a
 * QToolButton outside restyleActionGlyphs(), so it is inked by hand.
 *
 * Run with: ctest -R DebugFilterBarGlyphTest -V
 */

#include <QAction>
#include <QImage>
#include <QPalette>
#include <QToolButton>
#include <QtTest/QtTest>

#include "GroupedTest.h"
#include "Host.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "TDebug.h"
#include "TDebugFilterBar.h"
#include "mudlet.h"
#include "uiDesign.h"

using namespace std::chrono_literals;

class DebugFilterBarGlyphTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    const QString mHostname = qsl("Test-DebugFilterBarGlyph");

    // Any size settles it: both pictures are scaled from the resolution they were drawn at
    static constexpr int scmComparisonSize = 24;

    // TestPixels::inkOf() cannot report a difference in shape alone between two glyphs of one ink
    static QString differenceBetween(const QImage& left, const QImage& right)
    {
        if (left.size() != right.size()) {
            return qsl("they are not even the same size");
        }
        int differing = 0;
        for (int y = 0; y < left.height(); ++y) {
            for (int x = 0; x < left.width(); ++x) {
                if (left.pixel(x, y) != right.pixel(x, y)) {
                    ++differing;
                }
            }
        }
        return qsl("%1 of %2 pixels differ").arg(QString::number(differing), QString::number(left.width() * left.height()));
    }

    static QImage expectedGlyph(const QString& file, const QColor& ink) { return TestPixels::drawnAt(QIcon(uiDesign::tintedGlyph(uiDesign::glyphPixmap(file), ink)), scmComparisonSize); }

    static QImage expectedPauseGlyph() { return expectedGlyph(qsl(":/icons/debug-pause.svg"), uiDesign::themeTokens().mutedText); }

    // The bar's actions carry no object name, so they are found by their English text
    static QAction* barAction(const QString& text)
    {
        if (!mudlet::smpDebugFilterBar) {
            return nullptr;
        }
        for (auto* pAction : mudlet::smpDebugFilterBar->findChildren<QAction*>()) {
            if (pAction->text() == text) {
                return pAction;
            }
        }
        return nullptr;
    }

    static QToolButton* categoryButton()
    {
        if (!mudlet::smpDebugFilterBar) {
            return nullptr;
        }
        for (auto* pButton : mudlet::smpDebugFilterBar->findChildren<QToolButton*>()) {
            if (pButton->text() == qsl("Show")) {
                return pButton;
            }
        }
        return nullptr;
    }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }
    }

    void cleanupTestCase() { mSession.stop(); }

    void init()
    {
        QVERIFY2(mSession.startMudlet(), qPrintable(mSession.why()));
        // The buttons below are found by the words on them
        mudlet::self()->setInterfaceLanguage(qsl("en_US"));
    }

    void cleanup()
    {
        TDebug::setPaused(false);
        TDebug::discardPausedMessages();
        TDebug::smDebugMode = false;

        mSession.stop();
    }

    void test_thePauseButtonCarriesThePauseGlyphAndOffersToResumeOnceChecked()
    {
        startDebuggingProfile();

        QAction* pAction = barAction(qsl("Pause"));
        QVERIFY2(pAction, "the debug filter bar has no action reading 'Pause'");

        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QImage paused = TestPixels::drawnAt(pAction->icon(), scmComparisonSize, QIcon::Off);
        const QImage resuming = TestPixels::drawnAt(pAction->icon(), scmComparisonSize, QIcon::On);
        const QImage expectedPaused = expectedGlyph(qsl(":/icons/debug-pause.svg"), tokens.mutedText);
        const QImage expectedResuming = expectedGlyph(qsl(":/icons/debug-resume.svg"), tokens.accentText);
        qInfo().noquote() << qsl("  the Pause button is inked %1 unchecked and %2 checked").arg(TestPixels::inkOf(paused), TestPixels::inkOf(resuming));

        QVERIFY2(!paused.isNull(), "the Pause action has no picture at all");
        QVERIFY2(paused == expectedPaused, qPrintable(qsl("the Pause button is not the pause glyph in the quiet ink: %1").arg(differenceBetween(paused, expectedPaused))));
        QVERIFY2(resuming == expectedResuming, qPrintable(qsl("a checked Pause button is not the play glyph in the accent: %1").arg(differenceBetween(resuming, expectedResuming))));

        pAction->setChecked(true);
        const QImage whileHolding = TestPixels::drawnAt(pAction->icon(), scmComparisonSize, QIcon::On);
        const QString wordingWhileHolding = pAction->text();
        pAction->setChecked(false);

        QVERIFY2(whileHolding != paused,
                 qPrintable(qsl("pausing left the button showing the same picture it offered to pause with, at %1 - nothing says the console is holding messages back")
                                    .arg(TestPixels::inkOf(whileHolding))));
        QCOMPARE(wordingWhileHolding, qsl("Resume"));
    }

    void test_theClearButtonCarriesTheEraserGlyph()
    {
        startDebuggingProfile();

        QAction* pAction = barAction(qsl("Clear"));
        QVERIFY2(pAction, "the debug filter bar has no action reading 'Clear'");

        const QImage onTheBar = TestPixels::drawnAt(pAction->icon(), scmComparisonSize);
        const QImage expected = expectedGlyph(qsl(":/icons/debug-clear.svg"), uiDesign::themeTokens().mutedText);
        qInfo().noquote() << qsl("  the Clear button is inked %1 against an expected %2").arg(TestPixels::inkOf(onTheBar), TestPixels::inkOf(expected));

        QVERIFY2(!onTheBar.isNull(), "the Clear action has no picture at all");
        QVERIFY2(onTheBar == expected, qPrintable(qsl("the Clear button is not the eraser glyph in the toolbar's ink: %1").arg(differenceBetween(onTheBar, expected))));
    }

    void test_theShowButtonCarriesTheEditorsFilterGlyph()
    {
        startDebuggingProfile();

        QToolButton* pButton = categoryButton();
        QVERIFY2(pButton, "the debug filter bar has no tool button reading 'Show'");

        const QImage onTheBar = TestPixels::drawnAt(pButton->icon(), scmComparisonSize);
        const QImage expected = expectedGlyph(qsl(":/icons/editor-filter.svg"), uiDesign::themeTokens().mutedText);
        qInfo().noquote() << qsl("  the Show button is inked %1 against an expected %2").arg(TestPixels::inkOf(onTheBar), TestPixels::inkOf(expected));

        QVERIFY2(!onTheBar.isNull(), "the Show button has no picture at all");
        QVERIFY2(onTheBar == expected, qPrintable(qsl("the Show button is not the editor's funnel in the toolbar's ink: %1").arg(differenceBetween(onTheBar, expected))));
    }

    // A QIcon set on an action is re-derived by nothing
    void test_aPaletteChangeRetintsTheBar()
    {
        startDebuggingProfile();

        QAction* pAction = barAction(qsl("Pause"));
        QVERIFY2(pAction, "the debug filter bar has no action reading 'Pause'");
        const QPalette savedPalette = QApplication::palette();
        const QImage before = TestPixels::drawnAt(pAction->icon(), scmComparisonSize);

        QPalette movedPalette(savedPalette);
        const bool wasDark = savedPalette.color(QPalette::Window).lightness() < 128;
        movedPalette.setColor(QPalette::Window, wasDark ? QColor(0xec, 0xec, 0xec) : QColor(0x2c, 0x2c, 0x2e));
        movedPalette.setColor(QPalette::WindowText, wasDark ? QColor(Qt::black) : QColor(Qt::white));
        QApplication::setPalette(movedPalette);
        QTest::qWait(100ms);

        const QImage after = TestPixels::drawnAt(pAction->icon(), scmComparisonSize);
        const QImage expectedAfter = expectedPauseGlyph();
        qInfo().noquote() << qsl("  the Pause button was inked %1 and is now %2").arg(TestPixels::inkOf(before), TestPixels::inkOf(after));

        QApplication::setPalette(savedPalette);
        QTest::qWait(100ms);

        // Checked first: a restyle that did nothing at all could leave the pair below agreeing
        QVERIFY2(before != after, qPrintable(qsl("the palette moved and the bar's picture did not: it is still inked %1").arg(TestPixels::inkOf(after))));
        QVERIFY2(after == expectedAfter,
                 qPrintable(qsl("the bar was re-inked to %1 where the new palette's mutedText is %2, and %3")
                                    .arg(TestPixels::inkOf(after), TestPixels::inkOf(expectedAfter), differenceBetween(after, expectedAfter))));
    }

private:
    // The filter bar exists only once the Central Debug Console is attached, as toggling Debug does
    Host* startDebuggingProfile()
    {
        if (!mSession.openProfile(mHostname)) {
            QTest::qFail(qPrintable(mSession.why()), __FILE__, __LINE__);
            return nullptr;
        }
        auto* host = mSession.host();

        mudlet::self()->attachDebugArea(host->getName());
        TDebug::smDebugMode = true;
        TDebug::flushMessageQueue();
        return host;
    }
};

#include "DebugFilterBarGlyphTest.moc"
MUDLET_GROUPED_TEST_MAIN(DebugFilterBarGlyphTest)
