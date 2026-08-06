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

#include <QSignalSpy>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLabel.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lua.h>
#else
#include <lua.h>
#endif
}

using namespace std::chrono_literals;

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForWindowBackgroundTest();

// Covers the full-window background feature added in #9394: its interaction
// with lowerWindow(), with setBorderColor(), and the cost of its 'cover' mode.
class WindowBackgroundTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "WindowBackground-Test-Host";
    QString mPort;
    const QString mLocalhost = "localhost";
    QTemporaryDir mImageDir;

    QString writeImage(const QString& fileName, const QSize& size, const QColor& colour)
    {
        QImage image(size, QImage::Format_ARGB32);
        image.fill(colour);
        const QString path = mImageDir.filePath(fileName);
        if (!image.save(path, "PNG")) {
            return QString();
        }
        return path;
    }

    void runLua(const QString& script) { mpHost->getLuaInterpreter()->compileAndExecuteScript(script); }

    int luaInt(const QString& global)
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        lua_getglobal(L, global.toUtf8().constData());
        const int value = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        return value;
    }

    int stackIndex(const QWidget* widget) const { return mpHost->mpConsole->mpMainFrame->children().indexOf(widget); }

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForWindowBackgroundTest();

        QVERIFY(mImageDir.isValid());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        QTimer::singleShot(0ms, qApp, [this]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100ms);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), mHostname);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), mLocalhost);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), mPort);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(5000)) {
            QFAIL("Profile took too long to load.");
        }
        mpHost = mudlet::self()->getActiveHost();
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
        const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        delete mudlet::self();
        QDir(path).removeRecursively();
    }

    void init()
    {
        QVERIFY(mpHost);
        QVERIFY(mpHost->mpConsole);
        runLua(qsl("resetBackgroundImage('main', true)"));
        runLua(qsl("setBorderColor(0, 0, 0)"));
    }

    // lowerWindow() drops mpMainDisplay to the bottom of mpMainFrame's stack so a
    // lowered label still sits above the console. The full-window background is a
    // sibling in that same stack, so it has to stay below the display or it paints
    // over the whole console.
    void test_lowerWindowKeepsWindowBackgroundBottomMost()
    {
        const QString imagePath = writeImage(qsl("solid.png"), QSize(64, 64), Qt::red);
        QVERIFY(!imagePath.isEmpty());

        runLua(qsl("setBackgroundImage('main', [[%1]], 'cover', true)").arg(imagePath));
        QCOMPARE(mpHost->mpConsole->mWindowBgImageMode, 5);

        runLua(qsl("createLabel('lowerTarget', 10, 10, 100, 100, 1)"));
        QVERIFY(mpHost->mpConsole->mLabelMap.contains(qsl("lowerTarget")));

        runLua(qsl("lowerWindow('lowerTarget')"));

        QVERIFY2(stackIndex(mpHost->mpConsole->mpWindowBackground) < stackIndex(mpHost->mpConsole->mpMainDisplay), "lowerWindow() left the full-window background painting on top of the main display");
        QVERIFY2(stackIndex(mpHost->mpConsole->mpMainDisplay) < stackIndex(mpHost->mpConsole->mLabelMap.value(qsl("lowerTarget"))),
                 "lowerWindow() left the lowered label hidden behind the main display");

        runLua(qsl("deleteLabel('lowerTarget')"));
    }

    // Same ordering has to hold with no background image set, since the widget
    // exists either way.
    void test_lowerWindowOrderingHoldsWithoutABackgroundImage()
    {
        runLua(qsl("createLabel('lowerTarget', 10, 10, 100, 100, 1)"));
        runLua(qsl("lowerWindow('lowerTarget')"));

        QVERIFY(stackIndex(mpHost->mpConsole->mpWindowBackground) < stackIndex(mpHost->mpConsole->mpMainDisplay));
        QVERIFY(stackIndex(mpHost->mpConsole->mpMainDisplay) < stackIndex(mpHost->mpConsole->mLabelMap.value(qsl("lowerTarget"))));

        runLua(qsl("deleteLabel('lowerTarget')"));
    }

    // changeColors() rebuilds mpMainFrame's palette, and a game can reach it with
    // no user action at all through an OSC palette change.
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

    // A full-window background deliberately makes the frame transparent, so the
    // border colour has to be reported from where it was stored rather than read
    // back out of the palette.
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

    // 'cover' must crop the source down to the target aspect before scaling. The
    // scale-then-crop order allocates an intermediate proportional to the aspect
    // mismatch - 32400x1080 (~140MB) for a 3000x100 image in a 1920x1080 window -
    // on every resize event.
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

    // The crop has to pick the same region the scale-then-crop order arrived at,
    // i.e. the largest centred rectangle of the target's aspect ratio.
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

    // End to end: an extreme-aspect source still produces a brush exactly the size
    // of the background widget.
    void test_coverBrushMatchesWidgetSizeForExtremeAspectImage()
    {
        const QString imagePath = writeImage(qsl("wide.png"), QSize(3000, 100), Qt::green);
        QVERIFY(!imagePath.isEmpty());

        runLua(qsl("setBackgroundImage('main', [[%1]], 'cover', true)").arg(imagePath));

        const QSize widgetSize = mpHost->mpConsole->mpWindowBackground->size();
        QVERIFY(!widgetSize.isEmpty());
        QCOMPARE(mpHost->mpConsole->mpWindowBackground->palette().brush(QPalette::Window).texture().size(), widgetSize);
    }
};

void initializeQRCResourcesForWindowBackgroundTest()
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

#include "WindowBackgroundTest.moc"
QTEST_MAIN(WindowBackgroundTest)
