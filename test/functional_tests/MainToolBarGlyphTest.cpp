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
 * The main and detached toolbars draw the editor's glyph files, inked from the palette and
 * retinted when it moves. The Sound button's caption comes from iconText() and stays "Sound", so
 * its glyph alone says whether media is muted.
 *
 * Run with: ctest -R MainToolBarGlyphTest -V
 */

#include <QAction>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <QPalette>
#include <QSvgRenderer>
#include <QToolButton>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletMedia.h"
#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "TDetachedWindow.h"
#include "mudlet.h"
#include "uiDesign.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class MainToolBarGlyphTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    const QString mFirstHostname = qsl("MainToolBarGlyph-First");
    const QString mSecondHostname = qsl("MainToolBarGlyph-Second");

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

    static QAction* mainWindowTriggersAction() { return mudlet::self()->findChild<QAction*>(qsl("triggers_action")); }

    static QImage expectedTriggersGlyph()
    {
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        return TestPixels::drawnAt(QIcon(uiDesign::tintedGlyph(uiDesign::glyphPixmap(qsl(":/icons/editor-triggers.svg")), tokens.mutedText)), scmComparisonSize);
    }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.startMudlet(), qPrintable(mSession.why()));
        // The captions below are read as words rather than as pictures
        mudlet::self()->setInterfaceLanguage(qsl("en_US"));

        // Two of them, because slot_tabDetachRequested() refuses index 0
        QVERIFY2(mSession.openProfile(mFirstHostname), qPrintable(mSession.why()));
        QVERIFY2(mSession.openProfile(mSecondHostname), qPrintable(mSession.why()));
    }

    void cleanupTestCase() { mSession.stop(); }

    // Every other case compares two glyphs built the same way, so an SVG that renders empty
    // would leave both sides blank and pass them all; this one reads the ink itself
    void test_aGlyphIsDrawnFromItsSvgAtTheDesignsStrokeWidth()
    {
        const QImage drawn = uiDesign::glyphPixmap(qsl(":/icons/toolbar-map.svg")).toImage();
        QCOMPARE(drawn.size(), QSize(uiDesign::scmGlyphRasterSize, uiDesign::scmGlyphRasterSize));

        const auto inked = [](const QImage& image) {
            qint64 covered = 0;
            for (int y = 0; y < image.height(); ++y) {
                for (int x = 0; x < image.width(); ++x) {
                    if (qAlpha(image.pixel(x, y)) > 32) {
                        ++covered;
                    }
                }
            }
            return covered;
        };

        const qint64 drawnInk = inked(drawn);
        QVERIFY2(drawnInk > 0, "the glyph came out of its SVG with no ink in it at all");

        // Lucide authors every icon at stroke width 2, so the file drawn as shipped covers more of
        // the square than the design's lighter pen does
        QFile source(qsl(":/icons/toolbar-map.svg"));
        QVERIFY(source.open(QIODevice::ReadOnly));
        QSvgRenderer asAuthored(source.readAll());
        QVERIFY(asAuthored.isValid());
        QPixmap authored(uiDesign::scmGlyphRasterSize, uiDesign::scmGlyphRasterSize);
        authored.fill(Qt::transparent);
        QPainter painter(&authored);
        painter.setRenderHint(QPainter::Antialiasing);
        asAuthored.render(&painter);
        painter.end();

        QVERIFY2(uiDesign::scmGlyphStrokeWidth < 2.0, "this case only means anything while the design asks for a lighter pen than Lucide's");
        QVERIFY2(drawnInk < inked(authored.toImage()), "the glyph is as heavy as Lucide authored it, so scmGlyphStrokeWidth never reached the renderer");
    }

    void test_theSoundButtonKeepsItsCaptionWhileItsGlyphSaysTheState()
    {
        QToolButton* pButton = mudlet::self()->findChild<QToolButton*>(qsl("mute"));
        QVERIFY2(pButton, "the main window has no 'mute' toolbar button");
        QAction* pAction = mudlet::self()->findChild<QAction*>(qsl("muteMedia"));
        QVERIFY2(pAction, "the main window has no 'muteMedia' action");

        const bool startedMuted = MudletMedia::self()->allMuted();
        const QString captionUnmuted = pButton->text();
        const QString wordingUnmuted = pAction->text();
        const QImage glyphUnmuted = TestPixels::drawnAt(pButton->icon(), scmComparisonSize, pButton->isChecked() ? QIcon::On : QIcon::Off);

        MudletMedia::self()->toggleAllMuted();

        const QString captionMuted = pButton->text();
        const QString wordingMuted = pAction->text();
        const QImage glyphMuted = TestPixels::drawnAt(pButton->icon(), scmComparisonSize, pButton->isChecked() ? QIcon::On : QIcon::Off);
        const bool endedMuted = MudletMedia::self()->allMuted();

        MudletMedia::self()->toggleAllMuted();

        qInfo().noquote() << qsl("  caption \"%1\" -> \"%2\"; the action says \"%3\" -> \"%4\"").arg(captionUnmuted, captionMuted, wordingUnmuted, wordingMuted);
        qInfo().noquote() << qsl("  the glyph was %1 and is now %2").arg(TestPixels::inkOf(glyphUnmuted), TestPixels::inkOf(glyphMuted));

        QVERIFY2(!startedMuted && endedMuted, "toggleAllMuted() did not take the profiles from unmuted to muted, so nothing below was measured across the change");
        QVERIFY2(captionUnmuted == qsl("Sound"), qPrintable(qsl("the Sound button reads \"%1\" while media is unmuted").arg(captionUnmuted)));
        QVERIFY2(captionMuted == qsl("Sound"), qPrintable(qsl("the Sound button reads \"%1\" once media is muted - something is writing the action's wording onto the caption").arg(captionMuted)));
        QVERIFY2(!glyphUnmuted.isNull() && !glyphMuted.isNull(), "the Sound button has no picture in one of the two states");
        QVERIFY2(glyphUnmuted != glyphMuted,
                 qPrintable(qsl("muting left the Sound button's glyph exactly as it was, at %1 - the caption no longer says which state it is in, so the picture has to")
                                    .arg(TestPixels::inkOf(glyphMuted))));

        // A change of ink alone would pass the line above while the speaker kept its waves in both states
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        const QImage expectedUnmuted = TestPixels::drawnAt(QIcon(uiDesign::tintedGlyph(uiDesign::glyphPixmap(qsl(":/icons/toolbar-sound-on.svg")), tokens.mutedText)), scmComparisonSize);
        const QImage expectedMuted = TestPixels::drawnAt(QIcon(uiDesign::tintedGlyph(uiDesign::glyphPixmap(qsl(":/icons/toolbar-sound-off.svg")), tokens.accentText)), scmComparisonSize);
        QVERIFY2(glyphUnmuted == expectedUnmuted,
                 qPrintable(qsl("the unmuted Sound button is not the speaker-with-waves glyph in the quiet ink: %1").arg(differenceBetween(glyphUnmuted, expectedUnmuted))));
        QVERIFY2(glyphMuted == expectedMuted, qPrintable(qsl("the muted Sound button is not the speaker-with-x glyph in the accent: %1").arg(differenceBetween(glyphMuted, expectedMuted))));
    }

    void test_theTriggersButtonCarriesTheEditorsGlyph()
    {
        QAction* pAction = mainWindowTriggersAction();
        QVERIFY2(pAction, "the main window has no 'triggers_action' on its toolbar");
        const QImage onTheToolbar = TestPixels::drawnAt(pAction->icon(), scmComparisonSize);
        const QImage expected = expectedTriggersGlyph();
        qInfo().noquote() << qsl("  the Triggers button is inked %1 against an expected %2").arg(TestPixels::inkOf(onTheToolbar), TestPixels::inkOf(expected));

        QVERIFY2(!onTheToolbar.isNull(), "the Triggers toolbar action has no picture at all");
        QVERIFY2(onTheToolbar == expected,
                 qPrintable(qsl("the Triggers button is not the editor's glyph in the toolbar's ink: it is %1 where the editor's file tinted to mutedText is %2, and %3")
                                    .arg(TestPixels::inkOf(onTheToolbar), TestPixels::inkOf(expected), differenceBetween(onTheToolbar, expected))));
    }

    // Nothing re-derives a QIcon once it is set on an action
    void test_aPaletteChangeRetintsTheToolbar()
    {
        QAction* pAction = mainWindowTriggersAction();
        QVERIFY2(pAction, "the main window has no 'triggers_action' on its toolbar");
        const QPalette savedPalette = QApplication::palette();
        const QImage before = TestPixels::drawnAt(pAction->icon(), scmComparisonSize);

        QPalette movedPalette(savedPalette);
        const bool wasDark = savedPalette.color(QPalette::Window).lightness() < 128;
        movedPalette.setColor(QPalette::Window, wasDark ? QColor(0xec, 0xec, 0xec) : QColor(0x2c, 0x2c, 0x2e));
        movedPalette.setColor(QPalette::WindowText, wasDark ? QColor(Qt::black) : QColor(Qt::white));
        QApplication::setPalette(movedPalette);
        QTest::qWait(100ms);

        const QImage after = TestPixels::drawnAt(pAction->icon(), scmComparisonSize);
        const QImage expectedAfter = expectedTriggersGlyph();
        qInfo().noquote() << qsl("  the Triggers button was inked %1 and is now %2").arg(TestPixels::inkOf(before), TestPixels::inkOf(after));

        QApplication::setPalette(savedPalette);
        QTest::qWait(100ms);

        // Checked first: a restyle that did nothing at all could leave the pair below agreeing
        QVERIFY2(before != after, qPrintable(qsl("the palette moved and the toolbar's picture did not: it is still inked %1").arg(TestPixels::inkOf(after))));
        QVERIFY2(after == expectedAfter,
                 qPrintable(qsl("the toolbar was re-inked to %1 where the new palette's mutedText is %2, and %3")
                                    .arg(TestPixels::inkOf(after), TestPixels::inkOf(expectedAfter), differenceBetween(after, expectedAfter))));
    }

    void test_aDetachedWindowCarriesTheSameTriggersGlyph()
    {
        QVERIFY2(mudlet::self()->getDetachedWindows().isEmpty(), "a detached window was left over from an earlier test");
        mudlet::self()->slot_tabDetachRequested(1, QPoint(200, 200));
        TDetachedWindow* pDetachedWindow = mudlet::self()->getDetachedWindows().value(mSecondHostname);
        QVERIFY2(pDetachedWindow, qPrintable(qsl("detaching tab 1 produced no window for '%1' - the tab order is not what this test assumes").arg(mSecondHostname)));

        QAction* pDetachedAction = pDetachedWindow->findChild<QAction*>(qsl("triggers_action"));
        QVERIFY2(pDetachedAction, "the detached window has no 'triggers_action' on its toolbar");
        const QImage inTheDetachedWindow = TestPixels::drawnAt(pDetachedAction->icon(), scmComparisonSize);
        const QImage inTheMainWindow = TestPixels::drawnAt(mainWindowTriggersAction()->icon(), scmComparisonSize);
        qInfo().noquote() << qsl("  the detached window's Triggers button is inked %1 against the main window's %2").arg(TestPixels::inkOf(inTheDetachedWindow), TestPixels::inkOf(inTheMainWindow));

        mudlet::self()->slot_tabReattachRequested(mSecondHostname);

        QVERIFY2(!inTheDetachedWindow.isNull(), "the detached window's Triggers action has no picture at all");
        QVERIFY2(inTheDetachedWindow == inTheMainWindow,
                 qPrintable(qsl("a detached window's Triggers button is not the main window's picture: it is %1 where the main window's is %2, and %3")
                                    .arg(TestPixels::inkOf(inTheDetachedWindow), TestPixels::inkOf(inTheMainWindow), differenceBetween(inTheDetachedWindow, inTheMainWindow))));
    }
};

#include "MainToolBarGlyphTest.moc"
MUDLET_GROUPED_TEST_MAIN(MainToolBarGlyphTest)
