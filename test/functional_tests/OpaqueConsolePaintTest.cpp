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
            lua->compileAndExecuteScript(qsl("setBackgroundColor(%1, %2, %3)\n").arg(color.red()).arg(color.green()).arg(color.blue()));
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
        QVERIFY(host->mpConsole->setConsoleBackgroundImage(imagePath, 1));
        settle(pane);
        QVERIFY2(!pane->testAttribute(Qt::WA_OpaquePaintEvent), "a background image was painted over");
        QVERIFY(unpaintedPixels(pane) > 0);

        QVERIFY(host->mpConsole->resetConsoleBackgroundImage());
        settle(pane);
        QVERIFY(pane->testAttribute(Qt::WA_OpaquePaintEvent));
    }

    // Whatever the pane paints for itself has to match the background it hides.
    void test_paintingTheBackgroundLooksLikeBlendingOverIt()
    {
        TTextEdit* pane = startPane();
        QVERIFY(pane);
        Host* host = mudlet::self()->getActiveHost();
        auto* lua = host->getLuaInterpreter();
        lua->compileAndExecuteScript(qsl("setBackgroundColor(20, 40, 90)\n"
                                         "for i = 1, 60 do cecho(string.format('<red>LINE %d <white:blue>%s<reset> tail\\n', i, string.rep('#', i % 23))) end\n"));
        moveOffTheCellGrid(pane);
        settle(pane);
        QVERIFY(pane->testAttribute(Qt::WA_OpaquePaintEvent));

        pane->forceUpdate();
        const QImage painted = host->mpConsole->grab().toImage().convertToFormat(QImage::Format_RGB32);

        pane->setAttribute(Qt::WA_OpaquePaintEvent, false);
        pane->forceUpdate();
        const QImage blended = host->mpConsole->grab().toImage().convertToFormat(QImage::Format_RGB32);
        QVERIFY2(pane->testAttribute(Qt::WA_OpaquePaintEvent), "the pane did not go back to opaque painting after one frame");

        QCOMPARE(painted.size(), blended.size());
        // Antialiased ink rounds differently drawn straight onto a color than
        // drawn onto nothing and blended over it
        int worst = 0;
        for (int y = 0; y < painted.height(); ++y) {
            const auto* a = reinterpret_cast<const QRgb*>(painted.constScanLine(y));
            const auto* b = reinterpret_cast<const QRgb*>(blended.constScanLine(y));
            for (int x = 0; x < painted.width(); ++x) {
                worst = std::max({worst, std::abs(qRed(a[x]) - qRed(b[x])), std::abs(qGreen(a[x]) - qGreen(b[x])), std::abs(qBlue(a[x]) - qBlue(b[x]))});
            }
        }
        QVERIFY2(worst <= 2, qPrintable(qsl("painting the background differed from blending over it by %1 in one channel").arg(worst)));
    }

private:
    TTextEdit* startPane()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        Host* host = mudlet::self()->getActiveHost();
        if (!host || !host->mpConsole) {
            return nullptr;
        }
        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);
        // Short lines, so most of every row is background
        host->getLuaInterpreter()->compileAndExecuteScript(qsl("for i = 1, 60 do echo('LINE ' .. i .. '\\n') end\n"));
        qApp->processEvents();
        return host->mpConsole->mUpperPane;
    }

    // Off the cell grid at the right, so there is a sliver past the last whole
    // cell that the cached screen does not reach
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

    // Rendered onto magenta without the pane's own window background, so any
    // pixel the pane leaves alone stays magenta.
    static int unpaintedPixels(TTextEdit* pane)
    {
        QImage target(pane->size(), QImage::Format_ARGB32_Premultiplied);
        target.fill(Qt::magenta);
        pane->forceUpdate();
        pane->render(&target, QPoint(), QRegion(), QWidget::RenderFlags());
        int count = 0;
        const QRgb magenta = QColor(Qt::magenta).rgba();
        for (int y = 0; y < target.height(); ++y) {
            const auto* line = reinterpret_cast<const QRgb*>(target.constScanLine(y));
            count += std::count(line, line + target.width(), magenta);
        }
        return count;
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
