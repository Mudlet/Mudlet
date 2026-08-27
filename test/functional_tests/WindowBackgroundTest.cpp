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

#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLabel.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lua.h>
#else
#include <lua.h>
#endif
}

using namespace std::chrono_literals;

// Covers the full-window background feature added in #9394.
class WindowBackgroundTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "WindowBackground-Test-Host";
    QString mPort;
    const QString mLocalhost = "localhost";
    QTemporaryDir mImageDir;

    // a pattern rather than a flat fill, so that resampling differences show up
    QString writeImage(const QString& fileName, const QSize& size, const QColor& seed)
    {
        QImage image(size, QImage::Format_ARGB32);
        for (int y = 0; y < size.height(); ++y) {
            for (int x = 0; x < size.width(); ++x) {
                image.setPixel(x, y, qRgb((seed.red() + x * 7) % 256, (seed.green() + y * 13) % 256, (seed.blue() + (x + y) * 3) % 256));
            }
        }
        const QString path = mImageDir.filePath(fileName);
        if (!image.save(path, "PNG")) {
            return QString();
        }
        return path;
    }

    void runLua(const QString& script) { QVERIFY2(mpHost->getLuaInterpreter()->compileAndExecuteScript(script), qPrintable(script)); }

    int luaInt(const QString& global)
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        lua_getglobal(L, global.toUtf8().constData());
        const int value = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        return value;
    }

    QString luaString(const QString& global)
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        lua_getglobal(L, global.toUtf8().constData());
        const QString value = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        return value;
    }

    bool luaNil(const QString& global)
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        lua_getglobal(L, global.toUtf8().constData());
        const bool value = lua_isnil(L, -1);
        lua_pop(L, 1);
        return value;
    }

    int stackIndex(const QWidget* widget) const { return mpHost->mpConsole->mpMainFrame->children().indexOf(widget); }

    void verifyStackedBelow(const QWidget* lower, const QWidget* upper, const char* message)
    {
        QVERIFY(lower);
        QVERIFY(upper);
        const int lowerIndex = stackIndex(lower);
        const int upperIndex = stackIndex(upper);
        QVERIFY2(lowerIndex >= 0 && upperIndex >= 0, "a widget under test is not a child of mpMainFrame");
        QVERIFY2(lowerIndex < upperIndex, message);
    }

    QPixmap installedBackgroundBrush() const { return mpHost->mpConsole->mpWindowBackground->palette().brush(QPalette::Window).texture(); }

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

        QVERIFY(mImageDir.isValid());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }

        mudlet::self()->resize(1200, 800);
        QTest::qWait(100ms);
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
            delete mudlet::self();
            QDir(path).removeRecursively();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        QVERIFY(mpHost);
        QVERIFY(mpHost->mpConsole);
        QVERIFY(mpHost->mpConsole->mpWindowBackground);
        runLua(qsl("resetBackgroundImage('main', true)"));
        QCOMPARE(mpHost->mpConsole->mWindowBgImageMode, 0);
        runLua(qsl("setBorderColor(0, 0, 0)"));
    }

    // runs even when a QVERIFY aborts a test body, so nothing leaks into the next one
    void cleanup()
    {
        mpHost->mpConsole->deleteLabel(qsl("lowerTarget"));
        mpHost->mpConsole->deleteMiniConsole(qsl("lowerConsole"));
    }

    // lowerWindow() drops mpMainDisplay to the bottom of mpMainFrame's stack so a
    // lowered label still sits above the console - and the background is a sibling there.
    void test_lowerWindowKeepsWindowBackgroundBottomMost()
    {
        const QString imagePath = writeImage(qsl("solid.png"), QSize(64, 64), Qt::red);
        QVERIFY(!imagePath.isEmpty());

        runLua(qsl("setBackgroundImage('main', [[%1]], 'cover', true)").arg(imagePath));
        QCOMPARE(mpHost->mpConsole->mWindowBgImageMode, 5);

        runLua(qsl("createLabel('lowerTarget', 10, 10, 100, 100, 1)"));
        QVERIFY(mpHost->mpConsole->mLabelMap.contains(qsl("lowerTarget")));

        runLua(qsl("lowerWindow('lowerTarget')"));

        verifyStackedBelow(mpHost->mpConsole->mpWindowBackground, mpHost->mpConsole->mpMainDisplay, "lowerWindow() left the full-window background painting on top of the main display");
        verifyStackedBelow(mpHost->mpConsole->mpMainDisplay, mpHost->mpConsole->mLabelMap.value(qsl("lowerTarget")), "lowerWindow() left the lowered label hidden behind the main display");
    }

    // The six branches of lowerWindow() are copy-pasted, so cover a second one.
    void test_lowerWindowKeepsWindowBackgroundBottomMostForAMiniConsole()
    {
        const QString imagePath = writeImage(qsl("solidConsole.png"), QSize(64, 64), Qt::cyan);
        QVERIFY(!imagePath.isEmpty());

        runLua(qsl("setBackgroundImage('main', [[%1]], 'cover', true)").arg(imagePath));
        runLua(qsl("createMiniConsole('lowerConsole', 10, 10, 200, 100)"));
        QVERIFY(mpHost->mpConsole->mSubConsoleMap.contains(qsl("lowerConsole")));

        runLua(qsl("lowerWindow('lowerConsole')"));

        verifyStackedBelow(mpHost->mpConsole->mpWindowBackground, mpHost->mpConsole->mpMainDisplay, "lowerWindow() left the full-window background painting on top of the main display");
    }

    void test_lowerWindowOrderingHoldsWithoutABackgroundImage()
    {
        runLua(qsl("createLabel('lowerTarget', 10, 10, 100, 100, 1)"));
        runLua(qsl("lowerWindow('lowerTarget')"));

        verifyStackedBelow(mpHost->mpConsole->mpWindowBackground, mpHost->mpConsole->mpMainDisplay, "lowerWindow() put the main display below the full-window background widget");
        verifyStackedBelow(mpHost->mpConsole->mpMainDisplay, mpHost->mpConsole->mLabelMap.value(qsl("lowerTarget")), "lowerWindow() left the lowered label hidden behind the main display");
    }

    // a game can reach changeColors() with no user action, through an OSC palette change
    void test_borderColorSurvivesChangeColors()
    {
        runLua(qsl("setBorderColor(10, 20, 30)"));
        QCOMPARE(mpHost->mpConsole->mpMainFrame->palette().color(QPalette::Window), QColor(10, 20, 30));

        mpHost->mpConsole->changeColors();

        QCOMPARE(mpHost->mpConsole->mpMainFrame->palette().color(QPalette::Window), QColor(10, 20, 30));
        QCOMPARE(mpHost->mpConsole->borderColor(), QColor(10, 20, 30));
    }

    void test_borderColorSurvivesSetBackgroundColor()
    {
        runLua(qsl("setBorderColor(40, 50, 60)"));
        runLua(qsl("setBackgroundColor('main', 1, 2, 3, 255)"));

        QCOMPARE(mpHost->mpConsole->mpMainFrame->palette().color(QPalette::Window), QColor(40, 50, 60));
    }

    void test_borderColorReturnsAfterResettingTheBackground()
    {
        const QString imagePath = writeImage(qsl("reset.png"), QSize(64, 64), Qt::yellow);
        QVERIFY(!imagePath.isEmpty());

        runLua(qsl("setBorderColor(255, 0, 0)"));
        runLua(qsl("setBackgroundImage('main', [[%1]], 'cover', true)").arg(imagePath));
        QCOMPARE(mpHost->mpConsole->mpMainFrame->palette().color(QPalette::Window).alpha(), 0);

        runLua(qsl("resetBackgroundImage('main', true)"));

        QCOMPARE(mpHost->mpConsole->mpMainFrame->palette().color(QPalette::Window), QColor(255, 0, 0));
    }

    void test_setBorderColorUnderAFullWindowBackgroundKeepsTheFrameTransparent()
    {
        const QString imagePath = writeImage(qsl("order.png"), QSize(64, 64), Qt::white);
        QVERIFY(!imagePath.isEmpty());

        runLua(qsl("setBackgroundImage('main', [[%1]], 'cover', true)").arg(imagePath));
        runLua(qsl("setBorderColor(11, 22, 33)"));

        QCOMPARE(mpHost->mpConsole->mpMainFrame->palette().color(QPalette::Window).alpha(), 0);
        QCOMPARE(mpHost->mpConsole->borderColor(), QColor(11, 22, 33));
    }

    // the frame is transparent under a full-window background, so the palette cannot be the source
    void test_getBorderColorReportsSetValueUnderFullWindowBackground()
    {
        const QString imagePath = writeImage(qsl("solid2.png"), QSize(64, 64), Qt::blue);
        QVERIFY(!imagePath.isEmpty());

        runLua(qsl("setBorderColor(70, 80, 90)"));
        runLua(qsl("setBackgroundImage('main', [[%1]], 'cover', true)").arg(imagePath));
        runLua(qsl("borderR, borderG, borderB = getBorderColor()"));

        QCOMPARE(luaInt(qsl("borderR")), 70);
        QCOMPARE(luaInt(qsl("borderG")), 80);
        QCOMPARE(luaInt(qsl("borderB")), 90);

        QCOMPARE(mpHost->mpConsole->mpMainFrame->palette().color(QPalette::Window).alpha(), 0);
    }

    void test_coverSourceRectNeverExceedsTheSourceImage()
    {
        const QVector<QSize> sourceSizes{{3000, 100}, {100, 3000}, {1920, 1080}, {64, 64}, {1, 4000}, {4000, 1}};
        const QVector<QSize> targetSizes{{1920, 1080}, {800, 600}, {1, 1}, {3840, 40}};

        for (const QSize& source : sourceSizes) {
            for (const QSize& target : targetSizes) {
                const QRect crop = TConsole::coverSourceRect(source, target);
                const QString context = qsl("source %1x%2 target %3x%4").arg(source.width()).arg(source.height()).arg(target.width()).arg(target.height());
                QVERIFY2(!crop.isEmpty(), qPrintable(context));
                QVERIFY2(QRect(QPoint(0, 0), source).contains(crop), qPrintable(context));
            }
        }
    }

    void test_coverSourceRectMatchesAspectPreservingCentreCrop()
    {
        const QRect wideSource = TConsole::coverSourceRect(QSize(3000, 100), QSize(1920, 1080));
        QCOMPARE(wideSource.height(), 100);
        QCOMPARE(wideSource.width(), 177);
        QCOMPARE(wideSource.center().x(), QRect(0, 0, 3000, 100).center().x());

        const QRect tallSource = TConsole::coverSourceRect(QSize(100, 3000), QSize(1920, 1080));
        QCOMPARE(tallSource.width(), 100);
        QCOMPARE(tallSource.height(), 56);
        QCOMPARE(tallSource.center().y(), QRect(0, 0, 100, 3000).center().y());
    }

    void test_coverBrushMatchesWidgetSizeForExtremeAspectImage()
    {
        const QString imagePath = writeImage(qsl("wide.png"), QSize(3000, 100), Qt::green);
        QVERIFY(!imagePath.isEmpty());

        runLua(qsl("setBackgroundImage('main', [[%1]], 'cover', true)").arg(imagePath));

        const QSize widgetSize = mpHost->mpConsole->mpWindowBackground->size();
        QVERIFY(!widgetSize.isEmpty());
        QCOMPARE(installedBackgroundBrush().size(), widgetSize);
    }

    // the two orders resample differently, so this fails if the crop stops coming first
    void test_coverBrushIsScaledFromTheCroppedSourceRegion()
    {
        const QSize sourceSize(3000, 100);
        const QString imagePath = writeImage(qsl("order-wide.png"), sourceSize, Qt::darkGreen);
        QVERIFY(!imagePath.isEmpty());

        runLua(qsl("setBackgroundImage('main', [[%1]], 'cover', true)").arg(imagePath));

        const QSize widgetSize = mpHost->mpConsole->mpWindowBackground->size();
        const QPixmap source(imagePath);
        QCOMPARE(source.size(), sourceSize);
        const QPixmap expected = source.copy(TConsole::coverSourceRect(sourceSize, widgetSize)).scaled(widgetSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        QCOMPARE(installedBackgroundBrush().toImage(), expected.toImage());
    }

    void test_unloadableCoverImageIsReportedAndKeepsThePreviousBackground()
    {
        const QString goodPath = writeImage(qsl("good.png"), QSize(300, 200), Qt::gray);
        QVERIFY(!goodPath.isEmpty());

        runLua(qsl("setBackgroundImage('main', [[%1]], 'cover', true)").arg(goodPath));
        const QImage installed = installedBackgroundBrush().toImage();
        QVERIFY(!installed.isNull());

        runLua(qsl("bgOk, bgError = setBackgroundImage('main', [[%1]], 'cover', true)").arg(mImageDir.filePath(qsl("no-such-file.png"))));

        QVERIFY(luaNil(qsl("bgOk")));
        QVERIFY2(luaString(qsl("bgError")).contains(qsl("full window background image")), qPrintable(luaString(qsl("bgError"))));
        QCOMPARE(mpHost->mpConsole->mWindowBgImagePath, goodPath);
        QCOMPARE(installedBackgroundBrush().toImage(), installed);
    }

    void test_coverBrushFollowsAWindowResize()
    {
        const QString imagePath = writeImage(qsl("resize.png"), QSize(3000, 100), Qt::darkRed);
        QVERIFY(!imagePath.isEmpty());

        runLua(qsl("setBackgroundImage('main', [[%1]], 'cover', true)").arg(imagePath));
        const QSize sizeBefore = mpHost->mpConsole->mpWindowBackground->size();

        mudlet::self()->resize(900, 640);
        QTest::qWait(200ms);

        const QSize sizeAfter = mpHost->mpConsole->mpWindowBackground->size();
        QVERIFY2(sizeAfter != sizeBefore, "the window did not actually resize");
        QCOMPARE(installedBackgroundBrush().size(), sizeAfter);

        mudlet::self()->resize(1200, 800);
        QTest::qWait(200ms);
    }

    // clearing a stylesheet repolishes the widget, which can drop the palette brush
    void test_switchingFromStylesheetModeToCoverInstallsTheBrush()
    {
        const QString imagePath = writeImage(qsl("switch.png"), QSize(256, 128), Qt::magenta);
        QVERIFY(!imagePath.isEmpty());

        runLua(qsl("setBackgroundImage('main', [[%1]], 'border', true)").arg(imagePath));
        QVERIFY(!mpHost->mpConsole->mpWindowBackground->styleSheet().isEmpty());

        runLua(qsl("setBackgroundImage('main', [[%1]], 'cover', true)").arg(imagePath));

        QVERIFY(mpHost->mpConsole->mpWindowBackground->styleSheet().isEmpty());
        QCOMPARE(mpHost->mpConsole->mpWindowBackground->palette().brush(QPalette::Window).texture().size(), mpHost->mpConsole->mpWindowBackground->size());
    }
};

#include "WindowBackgroundTest.moc"
MUDLET_GROUPED_TEST_MAIN(WindowBackgroundTest)
