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

#include <QClipboard>
#include <QFileInfo>
#include <QFontDatabase>
#include <QPainter>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

// TTextEdit lays text out in cells of QFontMetrics::height(), which is a
// typographic measure rather than the glyph ink box. At a good number of font
// sizes the ink of a glyph such as "_" reaches a pixel past the bottom of its
// cell, so it only renders completely if nothing paints over that pixel
// afterwards. #9070 and #9719 are both reports of underscores vanishing because
// something did.
class GlyphOverflowTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "Test-GlyphOverflow";
    QString mPort;
    const QString mLocalhost = "localhost";

    // What the line below the underscores looks like. Each one takes a different
    // branch of the background fill in layoutGrapheme().
    struct Underlay
    {
        QString name;
        QString colourTag;
        bool selected = false;
    };

    static QVector<Underlay> underlays()
    {
        return {
                {qsl("the console background"), qsl("<white>")},
                {qsl("an explicit background colour"), qsl("<white:blue>")},
                {qsl("a bright background colour"), qsl("<black:yellow>")},
                {qsl("a selection"), qsl("<blue>"), true},
        };
    }

    static constexpr int kUnderscoreCount = 40;
    static constexpr int kFillerCount = 60;
    // Column used to sample what a pixel row looks like where no glyph was
    // drawn; whatever the line below paints there is the reference for its own
    // pixel row.
    static constexpr int kBackgroundSampleColumn = 50;
    // How far a channel has to move from its row's background before the pixel
    // counts as glyph ink rather than antialiasing noise.
    static constexpr int kInkThreshold = 24;
    // How far past the bottom of a cell to look for ink that overflowed out of it
    static constexpr int kOverflowScanRows = 4;
    static const inline QStringList kTestFamilies = {qsl("Bitstream Vera Sans Mono"), qsl("Ubuntu Mono")};
    static constexpr int kFirstSize = 9;
    static constexpr int kLastSize = 30;
    static constexpr int kSpaceRunCount = 20;
    // Any size where the bundled families are legible; these two cases are about
    // whether a decoration is drawn at all, not about how tall its cell is.
    static constexpr int kDecorationSize = 14;

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    static bool pixelIsInk(QRgb pixel, QRgb background)
    {
        return qAbs(qRed(pixel) - qRed(background)) > kInkThreshold || qAbs(qGreen(pixel) - qGreen(background)) > kInkThreshold || qAbs(qBlue(pixel) - qBlue(background)) > kInkThreshold;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own. Sharing the developer's
        // ~/.config/mudlet means sharing a profile list, so a second copy of
        // this test running at the same time is told the name it types is
        // already in use and never gets an enabled Connect button. Since #9712
        // the opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

#ifndef INCLUDE_FONTS
        QSKIP("Built with WITH_FONTS=NO, so the fonts whose metrics this measures are not available");
#else
        // src/main.cpp extracts the bundled fonts into the config directory and
        // FontManager picks them up from there, but no test binary runs it, so
        // on a machine that has not run Mudlet before there is nothing on disk
        // to pick up and Qt quietly substitutes another family.
        for (const QString& file : {qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf"),
                                    qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMoBd.ttf"),
                                    qsl(":/fonts/ubuntu-font-family-0.83/UbuntuMono-R.ttf"),
                                    qsl(":/fonts/ubuntu-font-family-0.83/UbuntuMono-B.ttf")}) {
            QVERIFY2(QFontDatabase::addApplicationFont(file) != -1, qPrintable(qsl("Could not register the bundled font %1").arg(file)));
        }
        for (const QString& family : kTestFamilies) {
            QVERIFY2(QFontDatabase::families().contains(family), qPrintable(qsl("'%1' is missing from the font database after registering the bundled files").arg(family)));
        }
#endif
    }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        // Port 0 asks the OS for an ephemeral port so parallel test runs do not
        // collide on a hardcoded one
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->isListening(), "TelnetServerStub failed to bind a loopback port");
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);
    }

    // A line has to render all of its ink whatever the line below it looks like,
    // and that ink has to match the same glyph drawn on its own with the same
    // font, cell geometry and painter flags.
    void test_lineBelowDoesNotEraseOverflowingInk()
    {
        Host* host = startOfflineProfile();
        QVERIFY2(host, "Could not start an offline profile");
        TTextEdit* pane = host->mpConsole->mUpperPane;
        QVERIFY2(pane, "No upper pane available");

        int sizesWithOverflow = 0;
        for (const QString& family : kTestFamilies) {
            for (int size = kFirstSize; size <= kLastSize; ++size) {
                applyFont(host, family, size);
                QVERIFY2(pane->getColumnCount() > kBackgroundSampleColumn,
                         qPrintable(qsl("%1 %2pt narrowed the pane to %3 columns, too few for the background sample at column %4")
                                            .arg(family)
                                            .arg(size)
                                            .arg(pane->getColumnCount())
                                            .arg(kBackgroundSampleColumn)));
                const int cellHeight = cellHeightOf(pane);
                const QPair<int, int> expected = referenceInkExtent(pane->font(), qsl("_"), cellWidthOf(pane), cellHeight);
                if (expected.second >= cellHeight) {
                    ++sizesWithOverflow;
                }

                for (const Underlay& underlay : underlays()) {
                    const QVector<QPoint> ink = renderAndCollectInk(host, underlay);
                    const QString where = qsl("%1 %2pt below %3").arg(family).arg(size).arg(underlay.name);
                    QVERIFY2(!ink.isEmpty(), qPrintable(qsl("%1: no underscore ink rendered at all").arg(where)));

                    const QPair<int, int> actual = inkExtent(ink);
                    QVERIFY2(actual == expected,
                             qPrintable(qsl("%1: underscore ink occupies %2 of its cell, expected %3 (cell is %4 tall)").arg(where, describeExtent(actual), describeExtent(expected)).arg(cellHeight)));
                }
            }
        }

        if (sizesWithOverflow == 0) {
            // The comparisons above all ran and passed, so this is a coverage
            // warning rather than a skip
            QWARN("None of the tested font sizes overflow their cell on this platform, so the overflow case went unexercised");
        }
    }

    // The screen is rendered into a pixmap sized from the number of whole
    // character cells that fit, so the bottom line's overflow only survives if
    // that pixmap has somewhere to put it.
    void test_bottomLineOverflowSurvivesThePixmapEdge()
    {
        Host* host = startOfflineProfile();
        QVERIFY2(host, "Could not start an offline profile");
        TTextEdit* pane = host->mpConsole->mUpperPane;
        QVERIFY2(pane, "No upper pane available");
        waitForQuietConsole(host);

        int checkedSizes = 0;
        for (int size = kFirstSize; size <= kLastSize; ++size) {
            applyFont(host, kTestFamilies.first(), size);
            const int cellHeight = cellHeightOf(pane);
            const int cellWidth = cellWidthOf(pane);
            const QPair<int, int> expected = referenceInkExtent(pane->font(), qsl("_"), cellWidth, cellHeight);
            if (expected.second < cellHeight) {
                continue; // this size keeps its ink inside the cell, nothing to check
            }
            // A pane whose height is an exact multiple of the cell height has no
            // pixel left over for the bottom line's overflow to appear in.
            if (pane->height() % cellHeight == 0) {
                continue;
            }

            const int screenHeight = pane->getScreenHeight();
            const QString underscores(kUnderscoreCount, QLatin1Char('_'));
            runLua(host, qsl("clearWindow()"));
            runLua(host, qsl("cecho('<white>' .. string.rep('filler\\n', %1) .. '%2\\n')").arg(screenHeight - 1).arg(underscores));
            pane->forceUpdate();
            QApplication::processEvents();

            // Where the line ended up, rather than where the echo above should have
            // put it: anything else printing to the console scrolls the view, which
            // otherwise silently moves a different row under the measurement.
            const int underscoreLine = findLine(host, underscores);
            QVERIFY2(underscoreLine >= 0, qPrintable(qsl("%1pt: the underscore line never reached the buffer").arg(size)));
            const int screenRow = underscoreLine - pane->imageTopLine();
            if (screenRow != screenHeight - 1) {
                QWARN(qPrintable(qsl("%1pt: the underscore line sits on row %2 rather than the bottom row %3, so the pixmap edge went unexercised at this size")
                                         .arg(size)
                                         .arg(screenRow)
                                         .arg(screenHeight - 1)));
                continue;
            }
            ++checkedSizes;

            const QImage rendered = renderPane(host);
            const int cellTop = screenRow * cellHeight;
            const QPair<int, int> actual = inkExtent(collectInk(rendered, cellTop, cellHeight, cellWidth));
            QVERIFY2(actual == expected,
                     qPrintable(qsl("%1pt: the bottom line's underscore ink occupies %2 of its cell, expected %3").arg(QString::number(size), describeExtent(actual), describeExtent(expected))));
        }

        if (checkedSizes == 0) {
            QSKIP("No font size produced an overflowing bottom line on this platform, so nothing here is being proved");
        }
    }

    // Repainting part of the pane clears whole character cells, which takes the
    // previous line's overflow pixel with it. That line sits outside the dirty
    // region and is not redrawn, so the pixel has to be put back explicitly.
    void test_partialRepaintKeepsTheLineAboveIntact()
    {
        Host* host = startOfflineProfile();
        QVERIFY2(host, "Could not start an offline profile");
        TTextEdit* pane = host->mpConsole->mUpperPane;
        QVERIFY2(pane, "No upper pane available");

        int checkedSizes = 0;
        for (int size = kFirstSize; size <= kLastSize; ++size) {
            applyFont(host, kTestFamilies.first(), size);
            const int cellHeight = cellHeightOf(pane);
            const int cellWidth = cellWidthOf(pane);
            const QPair<int, int> expected = referenceInkExtent(pane->font(), qsl("_"), cellWidth, cellHeight);
            if (expected.second < cellHeight) {
                continue;
            }
            ++checkedSizes;

            // More lines than fit, so imageTopLine() is past zero - the
            // precondition for the partial repaint below to reach
            // drawForeground()'s cached-pixmap path.
            runLua(host, qsl("clearWindow()"));
            runLua(host, qsl("cecho('<white>' .. string.rep('%1\\n', %2))").arg(QString(kUnderscoreCount, QLatin1Char('_'))).arg(pane->getScreenHeight() * 2));
            pane->forceUpdate();
            QApplication::processEvents();
            QVERIFY2(pane->imageTopLine() > 0, "The pane did not scroll, so the partial repaint would not reach the cached-pixmap path");

            // Simulate a partial repaint: the previous frame, the damaged band
            // reset to the console background, then only that band re-rendered.
            QImage rendered = renderPane(host);
            const int row = pane->getScreenHeight() / 2;
            const QRect damaged(0, row * cellHeight, pane->width(), cellHeight * 2);
            QPainter eraser(&rendered);
            eraser.fillRect(damaged, host->mpConsole->getConsoleBgColor());
            eraser.end();
            pane->render(&rendered, damaged.topLeft(), QRegion(damaged), QWidget::DrawChildren);

            // Only the bottom of the ink can be pinned here: every line is
            // underscores, so the top rows of the cell hold the overflow of the
            // line above it rather than this line's own glyph.
            const QPair<int, int> actual = inkExtent(collectInk(rendered, (row - 1) * cellHeight, cellHeight, cellWidth));
            QVERIFY2(actual.second == expected.second,
                     qPrintable(qsl("%1pt: after a partial repaint the line above the dirty region ends at row %2 of its cell, expected %3").arg(size).arg(actual.second).arg(expected.second)));
        }

        if (checkedSizes == 0) {
            QSKIP("No font size produced an overflowing line on this platform, so nothing here is being proved");
        }
    }

    // Scrolling reuses the cached screen by blitting it a whole number of cells
    // up or down, which lands a complete line of text in the strip below the
    // last one. Only the bottom line's own overflow belongs there.
    void test_scrollingLeavesNoGhostLineBelowTheBottomOne()
    {
        Host* host = startOfflineProfile();
        QVERIFY2(host, "Could not start an offline profile");
        TTextEdit* pane = host->mpConsole->mUpperPane;
        QVERIFY2(pane, "No upper pane available");

        int checkedSizes = 0;
        for (int size = kFirstSize; size <= kLastSize; ++size) {
            applyFont(host, kTestFamilies.first(), size);
            const int cellHeight = cellHeightOf(pane);
            const int cellWidth = cellWidthOf(pane);
            const QPair<int, int> expected = referenceInkExtent(pane->font(), qsl("_"), cellWidth, cellHeight);
            const int spareTop = pane->getScreenHeight() * cellHeight;
            if (expected.second < cellHeight || pane->height() - spareTop <= kOverflowScanRows) {
                continue;
            }
            ++checkedSizes;

            // Underscores for the overflow, letters past them so that a whole
            // ghost line would be unmistakable in the strip.
            const QString line = QString(kUnderscoreCount, QLatin1Char('_')) + QString(20, QLatin1Char('M'));
            runLua(host, qsl("clearWindow()"));
            runLua(host, qsl("cecho('<white>' .. string.rep('%1\\n', %2))").arg(line).arg(pane->getScreenHeight() * 4));
            pane->forceUpdate();
            QApplication::processEvents();
            // primes the cached screen the scroll below is blitted from
            renderPane(host);

            // drawForeground() ignores the cache entirely below ten scrolled-off
            // lines, so there has to be more scrollback than that
            const int topLineBeforeScroll = pane->imageTopLine();
            QVERIFY2(topLineBeforeScroll >= 10, "Not enough scrollback for drawForeground() to take its scrolling path");

            // Render straight after the scroll so the frame under test is the
            // one drawForeground() builds from the shifted cache.
            pane->scrollUp(3);
            QVERIFY2(pane->imageTopLine() < topLineBeforeScroll, "The pane did not scroll back, so the frame below is not built from a shifted cache");
            const QImage rendered = renderPane(host);

            for (int y = spareTop + kOverflowScanRows; y < pane->height(); ++y) {
                int litPixels = 0;
                for (int x = 0; x < rendered.width(); ++x) {
                    if (pixelIsInk(rendered.pixel(x, y), consoleBackground(host))) {
                        ++litPixels;
                    }
                }
                QVERIFY2(litPixels == 0, qPrintable(qsl("%1pt: %2 stray pixels %3 rows below the last character cell after scrolling back").arg(size).arg(litPixels).arg(y - spareTop)));
            }

            // As above, the top of the cell holds the previous line's overflow
            const QPair<int, int> actual = inkExtent(collectInk(rendered, spareTop - cellHeight, cellHeight, cellWidth));
            QVERIFY2(actual.second == expected.second,
                     qPrintable(qsl("%1pt: after scrolling back the bottom line's underscore ink ends at row %2 of its cell, expected %3").arg(size).arg(actual.second).arg(expected.second)));
        }

        if (checkedSizes == 0) {
            QSKIP("No font size produced an overflowing bottom line on this platform, so nothing here is being proved");
        }
    }

    // Miniconsoles keep the fill rule the main console does not, so check the
    // paint order protects their overflow too.
    void test_miniConsoleKeepsOverflowingInk()
    {
        Host* host = startOfflineProfile();
        QVERIFY2(host, "Could not start an offline profile");
        runLua(host, qsl("createMiniConsole('overflowMini', 0, 0, 800, 400)"));
        auto* mini = host->mpConsole->mSubConsoleMap.value(qsl("overflowMini"));
        QVERIFY2(mini, "The miniconsole was not created");
        TTextEdit* pane = mini->mUpperPane;
        QVERIFY2(pane, "The miniconsole has no pane");

        int checkedSizes = 0;
        for (int size = kFirstSize; size <= kLastSize; ++size) {
            runLua(host, qsl("setFont('overflowMini', '%1')").arg(kTestFamilies.first()));
            runLua(host, qsl("setMiniConsoleFontSize('overflowMini', %1)").arg(size));
            QApplication::processEvents();
            // this one goes through the miniconsole API rather than applyFont(),
            // so it needs its own check that the family was not substituted
            QVERIFY2(QFontInfo(pane->font()).family() == kTestFamilies.first(),
                     qPrintable(qsl("The miniconsole resolved to '%1' rather than '%2', so this would measure the wrong glyph").arg(QFontInfo(pane->font()).family(), kTestFamilies.first())));
            const int cellHeight = cellHeightOf(pane);
            const int cellWidth = cellWidthOf(pane);
            const QPair<int, int> expected = referenceInkExtent(pane->font(), qsl("_"), cellWidth, cellHeight);
            if (expected.second < cellHeight || pane->getColumnCount() <= kBackgroundSampleColumn) {
                continue;
            }
            ++checkedSizes;

            runLua(host, qsl("clearWindow('overflowMini')"));
            runLua(host, qsl("cecho('overflowMini', '<white>%1\\n')").arg(QString(kUnderscoreCount, QLatin1Char('_'))));
            runLua(host, qsl("cecho('overflowMini', '<white:blue>%1\\n')").arg(QString(kFillerCount, QLatin1Char(' '))));
            pane->forceUpdate();
            QApplication::processEvents();

            QImage rendered(pane->size(), QImage::Format_ARGB32_Premultiplied);
            rendered.fill(mini->getConsoleBgColor());
            pane->render(&rendered, QPoint(), QRegion(), QWidget::DrawChildren);

            const QPair<int, int> actual = inkExtent(collectInk(rendered, 0, cellHeight, cellWidth));
            QVERIFY2(actual == expected,
                     qPrintable(qsl("%1pt: a miniconsole's underscore ink occupies %2 of its cell, expected %3").arg(QString::number(size), describeExtent(actual), describeExtent(expected))));
        }

        if (checkedSizes == 0) {
            QSKIP("No font size produced an overflowing line in a miniconsole on this platform, so nothing here is being proved");
        }
    }

    // Copy-as-image sizes its pixmap at exactly one cell per selected line, so
    // the bottom line's overflow has nowhere to go unless the paint leaves room
    // for it and the image is trimmed back afterwards.
    void test_copyAsImageKeepsTheBottomLineOverflow()
    {
        Host* host = startOfflineProfile();
        QVERIFY2(host, "Could not start an offline profile");
        TTextEdit* pane = host->mpConsole->mUpperPane;
        QVERIFY2(pane, "No upper pane available");

        int checkedSizes = 0;
        for (int size = kFirstSize; size <= kLastSize; ++size) {
            applyFont(host, kTestFamilies.first(), size);
            const int cellHeight = cellHeightOf(pane);
            const int cellWidth = cellWidthOf(pane);
            const QPair<int, int> expected = referenceInkExtent(pane->font(), qsl("_"), cellWidth, cellHeight);
            if (expected.second < cellHeight) {
                continue;
            }
            ++checkedSizes;

            const int selectedLines = 5;
            runLua(host, qsl("clearWindow()"));
            runLua(host, qsl("cecho('<white>' .. string.rep('%1\\n', %2))").arg(QString(kUnderscoreCount, QLatin1Char('_'))).arg(selectedLines));
            pane->forceUpdate();
            QApplication::processEvents();

            selectRows(pane, 0, selectedLines - 1, cellHeight, cellWidth);
            QMetaObject::invokeMethod(pane, "slot_copySelectionToClipboardImage", Qt::DirectConnection);
            QApplication::processEvents();

            const QImage copied = QApplication::clipboard()->image();
            QVERIFY2(!copied.isNull(), qPrintable(qsl("%1pt: copy as image produced nothing").arg(size)));
            const QPair<int, int> actual = inkExtentOnFlat(copied, (selectedLines - 1) * cellHeight, cellHeight, consoleBackground(host));
            QVERIFY2(actual.second == expected.second,
                     qPrintable(qsl("%1pt: the copied image's bottom line ends at row %2 of its cell, expected %3").arg(size).arg(actual.second).arg(expected.second)));
            // the spare row must not survive as blank padding, nor bring an
            // extra line of text with it
            QVERIFY2(copied.height() <= selectedLines * cellHeight + kOverflowScanRows,
                     qPrintable(qsl("%1pt: the copied image is %2px tall for %3 lines of %4px").arg(size).arg(copied.height()).arg(selectedLines).arg(cellHeight)));
        }

        if (checkedSizes == 0) {
            QSKIP("No font size produced an overflowing line on this platform, so nothing here is being proved");
        }
    }

    // A blank cell has no glyph to draw, so its underline is the only thing that
    // reaches the screen from it. Qt draws that underline as part of the text
    // for an unlinked cell, which is what stops such a cell being skipped.
    void test_underlinedSpacesKeepTheirUnderline()
    {
        Host* host = startOfflineProfile();
        QVERIFY2(host, "Could not start an offline profile");
        QVERIFY2(host->mpConsole->mUpperPane, "No upper pane available");
        applyFont(host, kTestFamilies.first(), kDecorationSize);
        const QString spaces(kSpaceRunCount, QLatin1Char(' '));

        // Undecorated the same run is genuinely blank, so this pins the check
        // below to the underline rather than to anything else on the row
        runLua(host, qsl("clearWindow() echo('%1') echo('\\n')").arg(spaces));
        const int blankInk = inkOnLine(host, spaces);
        QVERIFY2(blankInk >= 0, "could not find the run of spaces in the buffer");
        QCOMPARE(blankInk, 0);

        runLua(host, qsl("clearWindow() setUnderline(true) echo('%1') setUnderline(false) echo('\\n')").arg(spaces));
        const int underlinedInk = inkOnLine(host, spaces);
        QVERIFY2(underlinedInk >= 0, "could not find the underlined run of spaces in the buffer");
        QVERIFY2(underlinedInk > 0, "an underlined run of spaces rendered no ink, so its underline was lost");
    }

    // A link suppresses Qt's own underline so a custom-coloured one can be drawn
    // separately, which means a linked blank cell is skipped and its underline
    // can only come from the decoration pass that follows the glyph.
    void test_linkedSpacesKeepTheirUnderline()
    {
        Host* host = startOfflineProfile();
        QVERIFY2(host, "Could not start an offline profile");
        QVERIFY2(host->mpConsole->mUpperPane, "No upper pane available");
        applyFont(host, kTestFamilies.first(), kDecorationSize);
        const QString spaces(kSpaceRunCount, QLatin1Char(' '));

        runLua(host, qsl("clearWindow() setUnderline(true) echoLink('%1', '', 'hint', true) setUnderline(false) echo('\\n')").arg(spaces));
        const int ink = inkOnLine(host, spaces);
        QVERIFY2(ink >= 0, "could not find the linked run of spaces in the buffer");
        QVERIFY2(ink > 0, "a linked, underlined run of spaces rendered no ink, so its underline was lost");
    }

    void cleanup()
    {
        const QString profilePath = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        deleteDirectory(profilePath);
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

private:
    // The pane paints its cells onto whatever the parent widget is showing, so
    // start from the console background rather than letting render() lay down
    // the default palette colour where no cell was filled.
    static QImage renderPane(Host* host)
    {
        TTextEdit* pane = host->mpConsole->mUpperPane;
        QImage image(pane->size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(host->mpConsole->getConsoleBgColor());
        pane->render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
        return image;
    }

    // For images too narrow to carry a background sample column, such as the
    // copy-as-image output, which is only as wide as the selected text.
    static QPair<int, int> inkExtentOnFlat(const QImage& image, int cellTop, int cellHeight, QRgb background)
    {
        int top = -1;
        int bottom = -1;
        for (int y = qMax(0, cellTop); y < qMin(image.height(), cellTop + cellHeight + kOverflowScanRows); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (pixelIsInk(image.pixel(x, y), background)) {
                    if (top < 0) {
                        top = y;
                    }
                    bottom = y;
                    break;
                }
            }
        }
        return {top - cellTop, bottom - cellTop};
    }

    static QRgb consoleBackground(Host* host) { return host->mpConsole->getConsoleBgColor().rgb(); }

    static int cellHeightOf(const TTextEdit* pane) { return QFontMetrics(pane->font()).height(); }
    static int cellWidthOf(const TTextEdit* pane) { return QFontMetrics(pane->font()).averageCharWidth(); }

    // First and last pixel row of a set of ink, as offsets from the cell top.
    // Empty ink reports {-1, -1} so a completely erased glyph never matches a
    // real reference extent.
    static QPair<int, int> inkExtent(const QVector<QPoint>& ink)
    {
        if (ink.isEmpty()) {
            return {-1, -1};
        }
        int top = ink.first().y();
        int bottom = top;
        for (const QPoint& point : ink) {
            top = qMin(top, point.y());
            bottom = qMax(bottom, point.y());
        }
        return {top, bottom};
    }

    static QString describeExtent(const QPair<int, int>& extent) { return qsl("rows %1..%2").arg(extent.first).arg(extent.second); }

    void applyFont(Host* host, const QString& family, int size)
    {
        QFont font(family, size);
        font.setFixedPitch(true);
        QVERIFY2(QFontInfo(font).family() == family, qPrintable(qsl("Qt substituted '%1' for the requested '%2', so this would measure the wrong glyph").arg(QFontInfo(font).family(), family)));
        const auto result = host->setDisplayFont(font);
        QVERIFY2(result.first, qPrintable(qsl("Could not set the display font to %1 %2pt: %3").arg(family).arg(size).arg(result.second)));
        QApplication::processEvents();
    }

    // Prints a line of underscores followed by a filler line dressed up as the
    // given underlay, repaints, and returns the ink of the underscore line.
    QVector<QPoint> renderAndCollectInk(Host* host, const Underlay& underlay)
    {
        TTextEdit* pane = host->mpConsole->mUpperPane;
        runLua(host, qsl("clearWindow()"));
        runLua(host, qsl("cecho('<white>%1\\n')").arg(QString(kUnderscoreCount, QLatin1Char('_'))));
        runLua(host, qsl("cecho('%1%2\\n')").arg(underlay.colourTag, QString(kFillerCount, QLatin1Char(' '))));

        const int underscoreLine = findLine(host, QString(kUnderscoreCount, QLatin1Char('_')));
        if (underscoreLine < 0) {
            return {};
        }
        if (underlay.selected) {
            if (underscoreLine + 1 >= static_cast<int>(host->mpConsole->buffer.buffer.size())) {
                return {};
            }
            auto& below = host->mpConsole->buffer.buffer.at(underscoreLine + 1);
            for (TChar& character : below) {
                character.select();
            }
        }
        pane->forceUpdate();
        QApplication::processEvents();

        const QImage rendered = renderPane(host);
        const int cellHeight = cellHeightOf(pane);
        return collectInk(rendered, (underscoreLine - pane->imageTopLine()) * cellHeight, cellHeight, cellWidthOf(pane));
    }

    // How many pixels of the row holding the given line differ from the console
    // background, sampled across the columns that line occupies. -1 if the line
    // is not in the buffer or has scrolled out of view.
    int inkOnLine(Host* host, const QString& lineText)
    {
        TTextEdit* pane = host->mpConsole->mUpperPane;
        const int line = findLine(host, lineText);
        if (line < 0) {
            return -1;
        }
        pane->forceUpdate();
        QApplication::processEvents();

        const QImage rendered = renderPane(host);
        const int cellHeight = cellHeightOf(pane);
        const int top = (line - pane->imageTopLine()) * cellHeight;
        if (top < 0) {
            return -1;
        }
        const QRgb background = host->mpConsole->getConsoleBgColor().rgb();
        const int lastY = qMin(top + cellHeight, rendered.height()) - 1;
        const int lastX = qMin(lineText.size() * cellWidthOf(pane), rendered.width()) - 1;
        int ink = 0;
        for (int y = top; y <= lastY; ++y) {
            for (int x = 0; x <= lastX; ++x) {
                if (pixelIsInk(rendered.pixel(x, y), background)) {
                    ++ink;
                }
            }
        }
        return ink;
    }

    // A profile keeps printing after signal_profileLoaded - the package manager
    // announces itself, for one - and anything arriving while a case is running
    // scrolls the view out from under the row being measured. Wait for the buffer
    // to stop growing before relying on where a line sits.
    static void waitForQuietConsole(Host* host)
    {
        int previousLastLine = -1;
        int pollsUnchanged = 0;
        while (pollsUnchanged < 3) {
            const int lastLine = host->mpConsole->buffer.getLastLineNumber();
            if (lastLine == previousLastLine) {
                ++pollsUnchanged;
            } else {
                pollsUnchanged = 0;
                previousLastLine = lastLine;
            }
            QTest::qWait(50);
        }
    }

    static int findLine(Host* host, const QString& text)
    {
        TBuffer& buffer = host->mpConsole->buffer;
        for (int i = 0; i <= buffer.getLastLineNumber(); ++i) {
            if (buffer.line(i) == text) {
                return i;
            }
        }
        return -1;
    }

    // Every pixel of the underscore run that differs from what its own pixel row
    // looks like away from the glyphs, as offsets from the cell's top left.
    // Reaches a few rows past the bottom of the cell so overflow is included.
    // That only avoids picking up the line below because every caller leaves it
    // blank or puts underscores on it, whose ink sits at the bottom of a cell.
    static QVector<QPoint> collectInk(const QImage& image, int cellTop, int cellHeight, int cellWidth)
    {
        QVector<QPoint> ink;
        const int sampleX = kBackgroundSampleColumn * cellWidth + cellWidth / 2;
        if (sampleX >= image.width() || cellTop < 0) {
            return ink;
        }
        const int lastX = qMin(kUnderscoreCount * cellWidth, image.width()) - 1;
        const int lastY = qMin(cellTop + cellHeight + kOverflowScanRows, image.height()) - 1;
        for (int y = cellTop; y <= lastY; ++y) {
            const QRgb background = image.pixel(sampleX, y);
            for (int x = 0; x <= lastX; ++x) {
                if (pixelIsInk(image.pixel(x, y), background)) {
                    ink.append(QPoint(x, y - cellTop));
                }
            }
        }
        return ink;
    }

    // Where the ink of a run of graphemes starts and ends relative to the top of
    // its cell, drawn cell by cell the way TTextEdit::paintGraphemeForeground()
    // draws it. The whole run is rendered rather than a single glyph because
    // neighbouring cells' antialiasing overlaps at the cell boundaries, which
    // moves the faintest row of the ink.
    static QPair<int, int> referenceInkExtent(const QFont& font, const QString& grapheme, int cellWidth, int cellHeight)
    {
        QImage image(kUnderscoreCount * cellWidth, cellHeight * 3, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::black);
        QPainter painter(&image);
        painter.setFont(font);
        painter.setPen(Qt::white);
        for (int cell = 0; cell < kUnderscoreCount; ++cell) {
            painter.drawText(QRect(cell * cellWidth, cellHeight, cellWidth, cellHeight), Qt::AlignCenter | Qt::TextDontClip | Qt::TextSingleLine, grapheme);
        }
        painter.end();

        int top = -1;
        int bottom = -1;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (pixelIsInk(image.pixel(x, y), qRgb(0, 0, 0))) {
                    if (top < 0) {
                        top = y;
                    }
                    bottom = y;
                    break;
                }
            }
        }
        return {top - cellHeight, bottom - cellHeight};
    }

    void runLua(Host* host, const QString& script) { QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(script), qPrintable(qsl("Lua script failed: %1").arg(script))); }

    Host* startOfflineProfile()
    {
        startProfile(mHostname, mLocalhost, mPort);
        auto* host = mudlet::self()->getActiveHost();
        if (!host) {
            return nullptr;
        }
        host->mEchoLuaErrors = true;

        mudlet::self()->resize(1400, 900);
        QApplication::processEvents();

        // cecho() into a live connection would race with the stub's traffic
        host->mTelnet.disconnectIt();
        if (!QTest::qWaitFor(
                    [host]() {
                        return host->mTelnet.getConnectionState() == QAbstractSocket::UnconnectedState;
                    },
                    5000)) {
            qWarning() << "Profile did not go offline in time; stub traffic may interleave with the printed lines";
        }
        return host;
    }

    // Starts a profile the way a user would via the GUI.
    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        if (!TestProfile::create(hostname, address, port)) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(mudlet::self()->getActiveHost()->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    // Drag-selects whole rows, which is the only way in from outside the class.
    static void selectRows(TTextEdit* pane, int firstRow, int lastRow, int cellHeight, int cellWidth)
    {
        auto send = [pane](QEvent::Type type, Qt::MouseButton button, Qt::MouseButtons buttons, const QPointF& pos) {
            QMouseEvent event(type, pos, pane->mapToGlobal(pos.toPoint()), button, buttons, Qt::NoModifier);
            QApplication::sendEvent(pane, &event);
        };
        const QPointF start(2, firstRow * cellHeight + 2);
        const QPointF end(kUnderscoreCount * cellWidth - 2, lastRow * cellHeight + cellHeight / 2);
        send(QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton, start);
        send(QEvent::MouseMove, Qt::NoButton, Qt::LeftButton, end);
        send(QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, end);
        QApplication::processEvents();
    }

    void deleteProfileDirectory(const QString& profileName) { deleteDirectory(mudlet::getMudletPath(enums::profileHomePath, profileName)); }

    void deleteDirectory(const QString& path)
    {
        QDir dir(path);
        if (!dir.exists()) {
            return;
        }
        dir.removeRecursively();
    }
};

#include "GlyphOverflowTest.moc"
MUDLET_GROUPED_TEST_MAIN(GlyphOverflowTest)
