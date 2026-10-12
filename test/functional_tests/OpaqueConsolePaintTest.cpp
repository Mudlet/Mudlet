/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// Unnamed namespace: FrontendRefreshSeamTest, in the same grouped binary, has its own PaintCounter
namespace {
class PaintCounter : public QObject
{
public:
    int count = 0;

protected:
    bool eventFilter(QObject*, QEvent* event) override
    {
        if (event->type() == QEvent::Paint) {
            ++count;
        }
        return false;
    }
};
} // namespace

/*
 * The main console's text pane paints its own background whenever nothing
 * could show through it, so that Qt need not repaint the widgets behind it on
 * every frame. These cases pin down when it may, that it then leaves no pixel
 * unpainted, and that the result looks no different.
 */
class OpaqueConsolePaintTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-OpaquePaint";
    QString mpPort;
    const QString mpLocalhost = "localhost";

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, 0);
        mpPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mpHostname);
    }

    void test_anOpaqueBackgroundLeavesNoPixelUnpainted()
    {
        TTextEdit* pane = startPane();
        QVERIFY(pane);
        auto* lua = mudlet::self()->getActiveHost()->getLuaInterpreter();
        lua->compileAndExecuteScript(qsl("setBackgroundColor(20, 40, 90)\n"));

        moveOffTheCellGrid(pane);
        settle(pane);
        QVERIFY2(pane->width() % pane->mFontWidth, "the pane was laid out back onto the cell grid");

        QVERIFY2(pane->testAttribute(Qt::WA_OpaquePaintEvent), "the pane still asks Qt to paint what lies beneath it");
        QCOMPARE(pane->cachedScreen().format(), QImage::Format_RGB32);
        QCOMPARE(unpaintedPixels(pane), 0);

        // The cached screen holds only the rows that can be seen, so it also
        // stops short of the bottom whenever part of the pane is covered
        pane->mScreenHeight -= 2;
        QCOMPARE(unpaintedPixels(pane), 0);

        // Too short for a single row, so there is nothing to draw over what Qt
        // has been told no longer shows
        pane->resize(pane->width(), pane->mFontHeight / 2);
        pane->mScreenHeight = 0;
        QCOMPARE(unpaintedPixels(pane), 0);
        QVERIFY2(!pane->testAttribute(Qt::WA_OpaquePaintEvent), "a pane with nothing to draw still covers what lies beneath it");
    }

    // The band a paint redraws is cleared to the console's background first, so
    // only the cells of that color may go unfilled.
    void test_aCellWithItsOwnBackgroundIsFilledOverTheClearedBand()
    {
        TTextEdit* pane = startPane();
        QVERIFY(pane);
        auto* lua = mudlet::self()->getActiveHost()->getLuaInterpreter();
        lua->compileAndExecuteScript(qsl("setBackgroundColor(20, 40, 90)\n"));
        const QString colouredLine = qsl("setBgColor(200, 30, 30) echo('XXXX') resetFormat() echo(' plain\\n')\n");
        const QRgb cellColor = QColor(200, 30, 30).rgb();
        // The X glyphs cover well under half of their four cells
        const qreal dpr = pane->devicePixelRatioF();
        const int twoCells = qRound(2 * pane->mFontWidth * pane->mFontHeight * dpr * dpr);

        lua->compileAndExecuteScript(colouredLine);
        settle(pane);
        QVERIFY(pane->testAttribute(Qt::WA_OpaquePaintEvent));
        const int afterFullRedraw = pixelsOf(pane->cachedScreen(), cellColor);
        QVERIFY2(afterFullRedraw >= twoCells, qPrintable(qsl("a full redraw left %1 pixels of the cells' own background, fewer than two cells' worth (%2)").arg(afterFullRedraw).arg(twoCells)));

        // And as the one new line a scroll leaves to draw
        lua->compileAndExecuteScript(colouredLine);
        pane->repaint();
        const int afterScroll = pixelsOf(pane->cachedScreen(), cellColor);
        QVERIFY2(afterScroll - afterFullRedraw >= twoCells,
                 qPrintable(qsl("drawing one more such line added %1 pixels of the cells' own background, fewer than two cells' worth (%2)").arg(afterScroll - afterFullRedraw).arg(twoCells)));
    }

    // Over an image the band is cleared to transparent, so the cells of the
    // console's own color still have to be filled to cover it.
    void test_cellsOfTheConsolesColorCoverABackgroundImage()
    {
        TTextEdit* pane = startPane();
        QVERIFY(pane);
        Host* host = mudlet::self()->getActiveHost();
        auto* lua = host->getLuaInterpreter();
        const QString imagePath = mConfigDir.filePath(qsl("cells.png"));
        QImage image(16, 16, QImage::Format_RGB32);
        image.fill(Qt::darkGreen);
        QVERIFY(image.save(imagePath));
        QVERIFY(host->mainConsoleView()->setConsoleBackgroundImage(imagePath, 1));
        const auto restore = qScopeGuard([host] {
            host->mainConsoleView()->resetConsoleBackgroundImage();
        });
        const qreal dpr = pane->devicePixelRatioF();
        const int tenCells = qRound(10 * pane->mFontWidth * pane->mFontHeight * dpr * dpr);

        // Black too, the default, as it is what a transparent clear holds once its alpha is dropped
        for (const QColor& color : {QColor(20, 40, 90), QColor(Qt::black)}) {
            lua->compileAndExecuteScript(qsl("setBackgroundColor(%1, %2, %3)\n"
                                             "setBgColor(%1, %2, %3) echo(string.rep(' ', 20)) resetFormat() echo('\\n')\n")
                                                 .arg(color.red())
                                                 .arg(color.green())
                                                 .arg(color.blue()));
            settle(pane);
            QVERIFY(!pane->testAttribute(Qt::WA_OpaquePaintEvent));
            const int filled = pixelsOf(pane->cachedScreen(), color.rgba());
            QVERIFY2(filled >= tenCells,
                     qPrintable(qsl("over a background image, the cells of the console's own color %1 left only %2 pixels filled, fewer than ten cells' worth (%3)")
                                        .arg(color.name())
                                        .arg(filled)
                                        .arg(tenCells)));
        }
    }

    // Ink reaching two or more lines down lands in cells whose backgrounds go down
    // after it, and a partial redraw starting below its line cannot put it back,
    // so a full redraw must not keep it either.
    void test_inkFromTwoLinesUpIsFilledOverLikeAnyRedrawWould()
    {
        TTextEdit* pane = startPane();
        QVERIFY(pane);
        Host* host = mudlet::self()->getActiveHost();
        auto* lua = host->getLuaInterpreter();
        // A family whose marks stack, unlike the bundled monospaced ones
        lua->compileAndExecuteScript(qsl("setBackgroundColor(20, 40, 90)\nsetFont('main', 'DejaVu Sans')\n"));
        settle(pane);
        // U+0E39 THAI CHARACTER SARA UU, stacked
        const QString deep = qsl("a") + QString(24, QChar(0x0E39));
        const int deepReach = inkBottomOf(pane->font(), deep, QRect(0, 0, pane->mFontWidth, pane->mFontHeight));
        if (deepReach <= 2 * pane->mFontHeight) {
            QSKIP(qPrintable(qsl("stacked marks reach only %1 pixels down a %2 pixel cell in %3").arg(deepReach).arg(pane->mFontHeight).arg(QFontInfo(pane->font()).family())));
        }
        lua->compileAndExecuteScript(qsl("echo('a' .. string.rep('\\224\\184\\185', 24) .. '\\n')\n"
                                         "for i = 1, 4 do echo(string.rep(' ', 10) .. '\\n') end\n"));
        settle(pane);
        QVERIFY(pane->testAttribute(Qt::WA_OpaquePaintEvent));
        const int deepLine = host->mainConsoleView()->buffer.lineBuffer.lastIndexOf(deep);
        QVERIFY(deepLine >= 0);
        const int row = deepLine - pane->imageTopLine();
        QVERIFY2(row >= 0 && row + 2 < pane->mScreenHeight, qPrintable(qsl("the stacked line is on row %1 of %2").arg(row).arg(pane->mScreenHeight)));

        const qreal dpr = pane->devicePixelRatioF();
        const QImage twoRowsDown = pane->cachedScreen().copy(QRect(0, qCeil((row + 2) * pane->mFontHeight * dpr), qFloor(3 * pane->mFontWidth * dpr), qFloor(pane->mFontHeight * dpr)));
        const int inked = twoRowsDown.width() * twoRowsDown.height() - pixelsOf(twoRowsDown, QColor(20, 40, 90).rgb());
        QVERIFY2(inked == 0, qPrintable(qsl("%1 pixels of the cells two rows below the stacked marks kept their ink").arg(inked)));
    }

    // A new background reaches the pane as an ordinary repaint, which would
    // otherwise keep every cached row but the last.
    void test_aNewBackgroundReachesEveryCachedRow()
    {
        TTextEdit* pane = startPane();
        QVERIFY(pane);
        auto* lua = mudlet::self()->getActiveHost()->getLuaInterpreter();
        lua->compileAndExecuteScript(qsl("for i = 1, 200 do echo('MORE ' .. i .. '\\n') end\n"
                                         "setBackgroundColor(20, 40, 90, 128)\n"));
        settle(pane);
        QVERIFY(!pane->testAttribute(Qt::WA_OpaquePaintEvent));
        QVERIFY2(pane->imageTopLine() >= 10, "the view is too near the top of the buffer for a repaint to reuse the cached screen");

        // From translucent and then from another opaque color, as the first
        // also changes the cached screen's format
        for (const QColor& color : {QColor(20, 40, 90), QColor(90, 20, 40)}) {
            lua->compileAndExecuteScript(qsl("setBackgroundColor(%1, %2, %3)\n"
                                             "setBgColor(%1, %2, %3) echo(string.rep(' ', 20)) resetFormat() echo('\\n')\n")
                                                 .arg(color.red())
                                                 .arg(color.green())
                                                 .arg(color.blue()));
            pane->repaint();
            pane->repaint();
            QVERIFY(pane->testAttribute(Qt::WA_OpaquePaintEvent));
            // Past the end of every line, where nothing but the background is
            const QImage cached = pane->cachedScreen();
            const int x = cached.width() - 1;
            for (int y = 0; y < cached.height(); ++y) {
                QVERIFY2(cached.pixel(x, y) == color.rgb(),
                         qPrintable(qsl("device row %1 of the cached screen still shows %2 rather than %3").arg(y).arg(QColor(cached.pixel(x, y)).name(), color.name())));
            }
        }
    }

    // Nothing holds a hover or selection repaint back until the new background
    // has been painted everywhere, so the first paint to see it may cover a
    // single row.
    void test_aNewBackgroundKeepsTheRowsAPartialRepaintMisses()
    {
        TTextEdit* pane = startPane();
        QVERIFY(pane);
        auto* lua = mudlet::self()->getActiveHost()->getLuaInterpreter();
        lua->compileAndExecuteScript(qsl("for i = 1, 200 do echo('MORE ' .. i .. '\\n') end\n"
                                         "setBackgroundColor(20, 40, 90)\n"));
        settle(pane);
        QVERIFY(pane->testAttribute(Qt::WA_OpaquePaintEvent));
        QVERIFY2(pane->imageTopLine() >= 10, "the view is too near the top of the buffer for a repaint to reuse the cached screen");

        lua->compileAndExecuteScript(qsl("setBackgroundColor(90, 20, 40)\n"));
        const QRect band(0, 2 * pane->mFontHeight, pane->width(), pane->mFontHeight);
        QImage shown(pane->size(), QImage::Format_ARGB32_Premultiplied);
        pane->render(&shown, band.topLeft(), QRegion(band), QWidget::RenderFlags());
        const QImage afterBand = pane->cachedScreen().copy();

        pane->forceUpdate();
        pane->repaint();
        QVERIFY2(afterBand == pane->cachedScreen(), "the rows outside the first repaint after a new background were left blank in the cached screen");
    }

    // The fill it paints while Qt is not painting beneath it is the opaque
    // version of a background that may since have become translucent.
    void test_aPaneWithNoRowsToDrawIsPaintedAgainOverWhatLiesBeneath()
    {
        TTextEdit* pane = startPane();
        QVERIFY(pane);
        mudlet::self()->getActiveHost()->getLuaInterpreter()->compileAndExecuteScript(qsl("setBackgroundColor(20, 40, 90)\n"));
        settle(pane);
        qApp->processEvents();
        QVERIFY(pane->testAttribute(Qt::WA_OpaquePaintEvent));

        PaintCounter paints;
        pane->installEventFilter(&paints);
        const int fontHeight = pane->mFontHeight;
        auto restore = qScopeGuard([&] {
            pane->mFontHeight = fontHeight;
            pane->removeEventFilter(&paints);
        });
        for (int before = -1; before != paints.count;) {
            before = paints.count;
            QTest::qWait(500ms);
        }
        pane->mFontHeight = 0;
        pane->repaint();
        QVERIFY(!pane->testAttribute(Qt::WA_OpaquePaintEvent));
        const int paintsSoFar = paints.count;
        QTRY_VERIFY2(paints.count > paintsSoFar, "the pane kept the solid fill it painted while nothing beneath it was painted");
    }

    // Hover and selection repaints copy rows between the cached screen and the
    // scratch buffer they draw in, so the two must share a format.
    void test_theScratchBufferFollowsTheCachedScreensFormat()
    {
        TTextEdit* pane = startPane();
        QVERIFY(pane);
        auto* lua = mudlet::self()->getActiveHost()->getLuaInterpreter();
        lua->compileAndExecuteScript(qsl("for i = 1, 200 do echo('MORE ' .. i .. '\\n') end\n"));
        const QRect band(0, 2 * pane->mFontHeight, pane->width(), pane->mFontHeight);
        for (const bool opaque : {true, false, true}) {
            lua->compileAndExecuteScript(opaque ? qsl("setBackgroundColor(20, 40, 90)\n") : qsl("setBackgroundColor(20, 40, 90, 128)\n"));
            settle(pane);
            QCOMPARE(pane->testAttribute(Qt::WA_OpaquePaintEvent), opaque);
            QVERIFY2(pane->imageTopLine() > 0, "the pane must be scrolled for a band repaint to reuse the cached screen");
            QVERIFY2(!pane->mMouseTracking && pane->mDirtyFirstLine < 0, "a drag or a pending dirty line would repaint the cache itself rather than the scratch buffer");

            QImage shown(pane->size(), QImage::Format_ARGB32_Premultiplied);
            shown.fill(Qt::magenta);
            pane->render(&shown, band.topLeft(), QRegion(band), QWidget::RenderFlags());
            QVERIFY2(!pane->mRenderBuffer.isNull(), "the band repaint did not use the scratch buffer, so nothing here tests it");
            QCOMPARE(pane->mRenderBuffer.format(), pane->cachedScreen().format());
        }
    }

    void test_aBackgroundThatCanShowThroughIsNotPaintedOver()
    {
        TTextEdit* pane = startPane();
        QVERIFY(pane);
        Host* host = mudlet::self()->getActiveHost();
        auto* lua = host->getLuaInterpreter();
        lua->compileAndExecuteScript(qsl("setBackgroundColor(20, 40, 90)\n"));
        settle(pane);
        QVERIFY(pane->testAttribute(Qt::WA_OpaquePaintEvent));

        lua->compileAndExecuteScript(qsl("setBackgroundColor(20, 40, 90, 128)\n"));
        settle(pane);
        QVERIFY2(!pane->testAttribute(Qt::WA_OpaquePaintEvent), "a translucent background was painted over what lies behind it");
        QCOMPARE(pane->cachedScreen().format(), QImage::Format_ARGB32_Premultiplied);
        QVERIFY(unpaintedPixels(pane) > 0);

        lua->compileAndExecuteScript(qsl("setBackgroundColor(20, 40, 90)\n"));
        settle(pane);
        QVERIFY2(pane->testAttribute(Qt::WA_OpaquePaintEvent), "an opaque background again did not bring back opaque painting");
        QCOMPARE(unpaintedPixels(pane), 0);

        const QString imagePath = mConfigDir.filePath(qsl("background.png"));
        QImage image(16, 16, QImage::Format_RGB32);
        image.fill(Qt::darkGreen);
        QVERIFY(image.save(imagePath));
        QVERIFY(host->mainConsoleView()->setConsoleBackgroundImage(imagePath, 1));
        settle(pane);
        QVERIFY2(!pane->testAttribute(Qt::WA_OpaquePaintEvent), "a background image was painted over");
        QVERIFY(unpaintedPixels(pane) > 0);

        QVERIFY(host->mainConsoleView()->resetConsoleBackgroundImage());
        settle(pane);
        QVERIFY(pane->testAttribute(Qt::WA_OpaquePaintEvent));
    }

    // Whatever the pane paints for itself has to match the background it hides.
    // Only backgrounds are compared: glyphs can rasterize differently onto an
    // opaque target than onto a transparent one, and on macOS they differ outright.
    void test_paintingTheBackgroundLooksLikeBlendingOverIt()
    {
        TTextEdit* pane = startPane();
        QVERIFY(pane);
        Host* host = mudlet::self()->getActiveHost();
        auto* lua = host->getLuaInterpreter();
        lua->compileAndExecuteScript(qsl("setBackgroundColor(20, 40, 90)\n"
                                         "clearWindow()\n"
                                         "for i = 1, 60 do cecho(string.format('<:red>%s<:blue>%s<reset>\\n', string.rep(' ', i % 7 + 1), string.rep(' ', i % 23))) end\n"));
        moveOffTheCellGrid(pane);
        settle(pane);
        QVERIFY(pane->testAttribute(Qt::WA_OpaquePaintEvent));

        pane->forceUpdate();
        const QImage painted = host->mainConsoleView()->grab().toImage().convertToFormat(QImage::Format_RGB32);

        pane->setAttribute(Qt::WA_OpaquePaintEvent, false);
        pane->forceUpdate();
        const QImage blended = host->mainConsoleView()->grab().toImage().convertToFormat(QImage::Format_RGB32);
        QVERIFY2(pane->testAttribute(Qt::WA_OpaquePaintEvent), "the pane did not go back to opaque painting after one frame");

        QCOMPARE(painted.size(), blended.size());
        // Only the pane's own pixels: grab() leaves whatever no widget paints
        // uninitialised, and that differs from one grab to the next (macOS)
        const qreal dpr = painted.devicePixelRatio();
        const QRect paneArea = QRect(pane->mapTo(host->mainConsoleView(), QPoint()) * dpr, pane->size() * dpr).intersected(painted.rect());
        QVERIFY(!paneArea.isEmpty());
        int worst = 0;
        QPoint worstAt;
        for (int y = paneArea.top(); y <= paneArea.bottom(); ++y) {
            const auto* a = reinterpret_cast<const QRgb*>(painted.constScanLine(y));
            const auto* b = reinterpret_cast<const QRgb*>(blended.constScanLine(y));
            for (int x = paneArea.left(); x <= paneArea.right(); ++x) {
                const int difference = std::max({std::abs(qRed(a[x]) - qRed(b[x])), std::abs(qGreen(a[x]) - qGreen(b[x])), std::abs(qBlue(a[x]) - qBlue(b[x]))});
                if (difference > worst) {
                    worst = difference;
                    worstAt = QPoint(x, y);
                }
            }
        }
        QVERIFY2(worst == 0,
                 qPrintable(qsl("painting the background differed from blending over it by %1 in one channel at (%2, %3), over %4: %5 against %6")
                                    .arg(worst)
                                    .arg(worstAt.x())
                                    .arg(worstAt.y())
                                    .arg(widgetAt(host->mainConsoleView(), worstAt / dpr), painted.pixelColor(worstAt).name(), blended.pixelColor(worstAt).name())));
    }

private:
    TTextEdit* startPane()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        Host* host = mudlet::self()->getActiveHost();
        if (!host || !host->mainConsoleView()) {
            return nullptr;
        }
        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);
        // Short lines, so most of every row is background
        host->getLuaInterpreter()->compileAndExecuteScript(qsl("for i = 1, 60 do echo('LINE ' .. i .. '\\n') end\n"));
        qApp->processEvents();
        return host->mainConsoleView()->mUpperPane;
    }

    // Off the cell grid at the right, so there is a sliver past the last whole
    // cell that the cached screen does not reach
    static QString widgetAt(QWidget* parent, const QPoint& at)
    {
        const QWidget* widget = parent->childAt(at);
        return widget ? qsl("%1 \"%2\"").arg(QLatin1String(widget->metaObject()->className()), widget->objectName()) : qsl("no child");
    }

    static void moveOffTheCellGrid(TTextEdit* pane)
    {
        const int columns = pane->width() / pane->mFontWidth;
        pane->resize(columns * pane->mFontWidth + pane->mFontWidth / 2, pane->height());
    }

    // A change of background only takes effect from the frame after the one
    // that first saw it.
    static void settle(TTextEdit* pane)
    {
        for (int frame = 0; frame < 2; ++frame) {
            pane->forceUpdate();
            pane->repaint();
        }
    }

    // How far below the top of the cell drawText() leaves ink, for the grapheme centered in it
    static int inkBottomOf(const QFont& font, const QString& grapheme, const QRect& cell)
    {
        const int above = cell.height() * 4;
        QImage image(cell.width() * 4, cell.height() * 12, QImage::Format_RGB32);
        image.fill(Qt::black);
        {
            QPainter painter(&image);
            painter.setFont(font);
            painter.setPen(Qt::white);
            painter.drawText(cell.translated(cell.width(), above), Qt::AlignCenter | Qt::TextDontClip | Qt::TextSingleLine, grapheme);
        }
        for (int y = image.height() - 1; y >= 0; --y) {
            const auto* line = reinterpret_cast<const QRgb*>(image.constScanLine(y));
            if (std::any_of(line, line + image.width(), [](const QRgb pixel) {
                    return qRed(pixel) > 0;
                })) {
                return y + 1 - above;
            }
        }
        return 0;
    }

    static int pixelsOf(const QImage& image, const QRgb color)
    {
        int count = 0;
        for (int y = 0; y < image.height(); ++y) {
            const auto* line = reinterpret_cast<const QRgb*>(image.constScanLine(y));
            count += std::count(line, line + image.width(), color);
        }
        return count;
    }

    // Rendered onto magenta without the pane's own window background, so any
    // pixel the pane leaves alone stays magenta.
    static int unpaintedPixels(TTextEdit* pane)
    {
        QImage target(pane->size(), QImage::Format_ARGB32_Premultiplied);
        target.fill(Qt::magenta);
        pane->forceUpdate();
        pane->render(&target, QPoint(), QRegion(), QWidget::RenderFlags());
        return pixelsOf(target, QColor(Qt::magenta).rgba());
    }

    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        auto host = TestProfile::create(hostname, address, port);
        if (!host) {
            QFAIL("No active host available for the test.");
        }
        QSignalSpy spy(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy.wait(2s)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName) { deleteDirectory(MudletApp::getMudletPath(enums::profileHomePath, profileName)); }

    void deleteDirectory(const QString& path)
    {
        QDir dir(path);
        if (!dir.exists()) {
            return;
        }
        dir.removeRecursively();
    }

private slots:
    void cleanup()
    {
        const QString profilePath = MudletApp::getMudletPath(enums::profileHomePath, mpHostname);
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        deleteDirectory(profilePath);
    }
};

#include "OpaqueConsolePaintTest.moc"
MUDLET_GROUPED_TEST_MAIN(OpaqueConsolePaintTest)
