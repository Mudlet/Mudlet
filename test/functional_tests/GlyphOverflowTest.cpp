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

#include <QPainter>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResources();

// TTextEdit lays text out in cells of QFontMetrics::height(), which is a
// typographic measure rather than the glyph ink box. At a good number of font
// sizes the ink of "_gjpqy$@()" reaches a pixel past the bottom of its cell, so
// those glyphs only render completely if nothing paints over that pixel
// afterwards. #9070 and #9719 are both reports of underscores vanishing because
// something did.
class GlyphOverflowTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "Test-GlyphOverflow";
    QString mPort; // assigned the stub's actual loopback port in init()
    const QString mLocalhost = "localhost";

    // What the line below the underscores looks like. Each of these used to
    // paint over the overflow pixel of the line above it.
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
    // drawn: past the underscores, still inside the filler line.
    static constexpr int kBackgroundSampleColumn = 50;
    // How far a channel has to move from its row's background before the pixel
    // counts as glyph ink rather than antialiasing noise.
    static constexpr int kInkThreshold = 24;
    // How far past the bottom of a cell to look for ink that overflowed out of it
    static constexpr int kOverflowScanRows = 4;
    static constexpr int kFirstSize = 9;
    static constexpr int kLastSize = 30;

    static bool pixelIsInk(QRgb pixel, QRgb background)
    {
        return qAbs(qRed(pixel) - qRed(background)) > kInkThreshold || qAbs(qGreen(pixel) - qGreen(background)) > kInkThreshold || qAbs(qBlue(pixel) - qBlue(background)) > kInkThreshold;
    }

private slots:
    void initTestCase() { initializeQRCResources(); }

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
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);
    }

    // A line has to render all of its ink whatever the line below it looks like,
    // and that ink has to match the glyph drawn on its own with the same font,
    // cell geometry and painter flags.
    void test_lineBelowDoesNotEraseOverflowingInk()
    {
        Host* host = startOfflineProfile();
        TTextEdit* pane = host->mpConsole->mUpperPane;
        QVERIFY2(pane, "No upper pane available");

        int sizesWithOverflow = 0;
        for (const QString& family : {qsl("Bitstream Vera Sans Mono"), qsl("Ubuntu Mono")}) {
            for (int size = kFirstSize; size <= kLastSize; ++size) {
                applyFont(host, family, size);
                const int cellHeight = cellHeightOf(pane);
                const int expectedBottom = referenceInkBottom(pane->font(), qsl("_"), cellWidthOf(pane), cellHeight);
                if (expectedBottom >= cellHeight) {
                    ++sizesWithOverflow;
                }

                for (const Underlay& underlay : underlays()) {
                    const QVector<QPoint> ink = renderAndCollectInk(host, underlay);
                    const QString where = qsl("%1 %2pt below %3").arg(family).arg(size).arg(underlay.name);
                    QVERIFY2(!ink.isEmpty(), qPrintable(qsl("%1: no underscore ink rendered at all").arg(where)));

                    int actualBottom = ink.first().y();
                    for (const QPoint& point : ink) {
                        actualBottom = qMax(actualBottom, point.y());
                    }
                    QVERIFY2(actualBottom == expectedBottom,
                             qPrintable(qsl("%1: underscore ink ends %2 rows below the cell top, expected %3 (cell is %4 tall)").arg(where).arg(actualBottom).arg(expectedBottom).arg(cellHeight)));
                }
            }
        }

        QVERIFY2(sizesWithOverflow > 0, "None of the tested font sizes overflow their cell, so this test proves nothing - pick different sizes");
    }

    // The screen is rendered into a pixmap sized from the number of whole
    // character cells that fit, so the bottom line's overflow used to be cut off
    // by the edge of that pixmap regardless of what was painted afterwards.
    void test_bottomLineOverflowSurvivesThePixmapEdge()
    {
        Host* host = startOfflineProfile();
        TTextEdit* pane = host->mpConsole->mUpperPane;
        QVERIFY2(pane, "No upper pane available");

        int checkedSizes = 0;
        for (int size = kFirstSize; size <= kLastSize; ++size) {
            applyFont(host, qsl("Bitstream Vera Sans Mono"), size);
            const int cellHeight = cellHeightOf(pane);
            const int cellWidth = cellWidthOf(pane);
            const int expectedBottom = referenceInkBottom(pane->font(), qsl("_"), cellWidth, cellHeight);
            if (expectedBottom < cellHeight) {
                continue; // this size keeps its ink inside the cell, nothing to check
            }
            // A pane whose height is an exact multiple of the cell height has no
            // pixel left over for the bottom line's overflow to appear in.
            if (pane->height() % cellHeight == 0) {
                continue;
            }
            ++checkedSizes;

            const int screenHeight = pane->getScreenHeight();
            runLua(host, qsl("clearWindow()"));
            runLua(host, qsl("cecho('<white>' .. string.rep('filler\\n', %1) .. '%2\\n')").arg(screenHeight - 1).arg(QString(kUnderscoreCount, QLatin1Char('_'))));
            pane->forceUpdate();
            QApplication::processEvents();

            const QImage rendered = renderPane(host);
            const int cellTop = (screenHeight - 1) * cellHeight;
            const int actualBottom = inkBottom(collectInk(rendered, cellTop, cellHeight, cellWidth));
            QVERIFY2(actualBottom == expectedBottom,
                     qPrintable(qsl("%1pt: the bottom line's underscore ink ends %2 rows below the cell top, expected %3").arg(size).arg(actualBottom).arg(expectedBottom)));
        }

        QVERIFY2(checkedSizes > 0, "No font size produced an overflowing bottom line, so this test proves nothing");
    }

    // Repainting part of the pane clears whole character cells, which takes the
    // previous line's overflow pixel with it. That line sits outside the dirty
    // region and is not redrawn, so the pixel has to be put back explicitly.
    void test_partialRepaintKeepsTheLineAboveIntact()
    {
        Host* host = startOfflineProfile();
        TTextEdit* pane = host->mpConsole->mUpperPane;
        QVERIFY2(pane, "No upper pane available");

        int checkedSizes = 0;
        for (int size = kFirstSize; size <= kLastSize; ++size) {
            applyFont(host, qsl("Bitstream Vera Sans Mono"), size);
            const int cellHeight = cellHeightOf(pane);
            const int cellWidth = cellWidthOf(pane);
            const int expectedBottom = referenceInkBottom(pane->font(), qsl("_"), cellWidth, cellHeight);
            if (expectedBottom < cellHeight) {
                continue;
            }
            ++checkedSizes;

            // More lines than fit, so the pane has scrolled and drawForeground()
            // is on its cached-pixmap path.
            runLua(host, qsl("clearWindow()"));
            runLua(host, qsl("cecho('<white>' .. string.rep('%1\\n', %2))").arg(QString(kUnderscoreCount, QLatin1Char('_'))).arg(pane->getScreenHeight() * 2));
            pane->forceUpdate();
            QApplication::processEvents();

            // What the user sees after a partial repaint: the previous frame,
            // with the damaged band erased to the console background by Qt and
            // then painted over by the widget.
            QImage rendered = renderPane(host);
            const int row = pane->getScreenHeight() / 2;
            const QRect damaged(0, row * cellHeight, pane->width(), cellHeight * 2);
            QPainter eraser(&rendered);
            eraser.fillRect(damaged, host->mpConsole->getConsoleBgColor());
            eraser.end();
            pane->render(&rendered, damaged.topLeft(), QRegion(damaged), QWidget::DrawChildren);

            const int actualBottom = inkBottom(collectInk(rendered, (row - 1) * cellHeight, cellHeight, cellWidth));
            QVERIFY2(actualBottom == expectedBottom,
                     qPrintable(qsl("%1pt: after a partial repaint the line above the dirty region ends %2 rows below its cell top, expected %3").arg(size).arg(actualBottom).arg(expectedBottom)));
        }

        QVERIFY2(checkedSizes > 0, "No font size produced an overflowing line, so this test proves nothing");
    }

    void cleanup()
    {
        const QString profilePath = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        deleteDirectory(profilePath);
    }

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

    static int cellHeightOf(const TTextEdit* pane) { return QFontMetrics(pane->font()).height(); }
    static int cellWidthOf(const TTextEdit* pane) { return QFontMetrics(pane->font()).averageCharWidth(); }

    static int inkBottom(const QVector<QPoint>& ink)
    {
        int bottom = -1;
        for (const QPoint& point : ink) {
            bottom = qMax(bottom, point.y());
        }
        return bottom;
    }

    void applyFont(Host* host, const QString& family, int size)
    {
        QFont font(family, size);
        font.setFixedPitch(true);
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
    // Reaches a few rows past the bottom of the cell so overflow is included
    // without picking up the glyphs of the line below.
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

    // Where the glyph's ink ends relative to the top of its cell when drawn the
    // way TTextEdit::paintGraphemeForeground() draws it.
    static int referenceInkBottom(const QFont& font, const QString& grapheme, int cellWidth, int cellHeight)
    {
        QImage image(cellWidth, cellHeight * 3, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::black);
        QPainter painter(&image);
        painter.setFont(font);
        painter.setPen(Qt::white);
        painter.drawText(QRect(0, cellHeight, cellWidth, cellHeight), Qt::AlignCenter | Qt::TextDontClip | Qt::TextSingleLine, grapheme);
        painter.end();

        int bottom = -1;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (pixelIsInk(image.pixel(x, y), qRgb(0, 0, 0))) {
                    bottom = y;
                    break;
                }
            }
        }
        return bottom - cellHeight;
    }

    void runLua(Host* host, const QString& script) { host->getLuaInterpreter()->compileAndExecuteScript(script); }

    Host* startOfflineProfile()
    {
        startProfile(mHostname, mLocalhost, mPort);
        auto* host = mudlet::self()->getActiveHost();
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

    // Starts a profile the way a user would via the GUI (mirrors the helper in
    // TelnetTextDisplayedTest).
    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        QTimer::singleShot(0, qApp, [hostname, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), hostname);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), address);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(5000)) {
            QFAIL("Profile took too long to load.");
        }
        if (!mudlet::self()->getActiveHost()) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(mudlet::self()->getActiveHost()->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
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

void initializeQRCResources()
{
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
    qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
    qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    qInitResources_mudlet_fonts_posix();
#endif
#endif
    qInitResources_mudlet();
    qInitResources_qm();
}

#include "GlyphOverflowTest.moc"
QTEST_MAIN(GlyphOverflowTest)
