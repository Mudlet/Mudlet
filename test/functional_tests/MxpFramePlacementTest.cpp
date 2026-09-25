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
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TMxpFrameManager.h"
#include "TMxpFrameWidgets.h"
#include "TPrintSink.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// Covers issue #9698: a package that reserves space with setBorderRight() and
// friends - the base UI does exactly that - must not have MXP frames placed on
// top of the space it claimed.
class MxpFramePlacementTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "MxpFramePlacement-Test-Host";
    QString mPort;
    const QString mLocalhost = "localhost";

    void runLua(const QString& script) { QVERIFY2(mpHost->getLuaInterpreter()->compileAndExecuteScript(script), qPrintable(script)); }

    // The space frames may be placed in: the main window, less whatever a
    // package has reserved for itself. This repeats TMxpFrameManager's own
    // formula, so tests that need an anchor independent of it assert against a
    // literal or against another widget's geometry instead.
    QRect area() const { return QRect(QPoint(0, 0), mpHost->mpConsole->getMainWindowSize()).marginsRemoved(mpHost->userBorders()); }

    QWidget* frameWidget(const QString& name) const { return mpHost->mpConsole->mxpFrameWidgets().frameWidget(name); }
    TConsole* frameConsole(const QString& name) const { return mpHost->mpConsole->mxpFrameWidgets().frameConsole(name); }
    QTabWidget* frameTabs(const QString& name) const { return mpHost->mpConsole->mxpFrameWidgets().frameTabs(name); }

    QRect frameGeometry(const QString& name) const
    {
        const QWidget* widget = frameWidget(name);
        return widget ? widget->geometry() : QRect();
    }

    bool createFrame(const QString& name, const QString& align, const QString& width, const QString& height, const QMap<QString, QString>& extraAttributes = {})
    {
        QMap<QString, QString> attributes = extraAttributes;
        attributes.insert(qsl("NAME"), name);
        attributes.insert(qsl("ALIGN"), align);
        if (!width.isEmpty()) {
            attributes.insert(qsl("WIDTH"), width);
        }
        if (!height.isEmpty()) {
            attributes.insert(qsl("HEIGHT"), height);
        }
        const bool created = mpHost->mMxpFrameManager.createFrame(name, attributes);
        settle();
        return created;
    }

    // border changes and window resizes reposition frames from a zero timer
    void settle() { QTest::qWait(50ms); }

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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        // Sized before the profile, so before there is a TMainConsole to resize:
        // getMainWindowSize() ignores a shrink of more than half and goes on
        // reporting the size from before it, and every later report is measured
        // against that kept size, so it never catches up. Whatever the platform
        // picks for a main window nobody has sized is not ours to rely on - under
        // the offscreen plugin it is over 16000 pixels wide on some machines, and
        // coming down from that to 1200 is exactly the drop that gets ignored.
        mudlet::self()->resize(1200, 800);

        QDir(MudletApp::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

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

        // Frames are placed against getMainWindowSize(), so a run where that has
        // stopped tracking the window would fail every placement assertion with
        // numbers that say nothing about frame placement. It only ever subtracts
        // from the console's own size, so anything bigger means it is reporting a
        // size the window no longer has - say so here instead.
        const QSize reported = mpHost->mpConsole->getMainWindowSize();
        const QSize consoleSize = mpHost->mpConsole->size();
        QVERIFY2(reported.width() <= consoleSize.width() && reported.height() <= consoleSize.height(),
                 qPrintable(qsl("getMainWindowSize() reports %1x%2 inside a console that is only %3x%4")
                                    .arg(QString::number(reported.width()), QString::number(reported.height()), QString::number(consoleSize.width()), QString::number(consoleSize.height()))));
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            const QString path = MudletApp::getMudletPath(enums::profileHomePath, mHostname);
            delete mudlet::self();
            QDir(path).removeRecursively();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        QVERIFY(mpHost);
        QVERIFY(mpHost->mpConsole);
        mpHost->mMxpProcessor.enable();
        mudlet::self()->resize(1200, 800);
        settle();
    }

    // runs even when a QVERIFY aborts a test body, so no state carries into the
    // next test - or, through the window geometry Mudlet saves on exit, into the
    // next run of this binary
    void cleanup()
    {
        mpHost->mMxpFrameManager.resetAllFrames();
        runLua(qsl("setBorderSizes(0)"));
        mudlet::self()->resize(1200, 800);
        settle();
    }

    void test_rightFrameKeepsClearOfAReservedRightBorder()
    {
        runLua(qsl("setBorderRight(300)"));
        settle();
        const QRect reservedArea = area();

        QVERIFY(createFrame(qsl("status"), qsl("right"), qsl("200px"), qsl("100%")));

        const QRect frame = frameGeometry(qsl("status"));
        QCOMPARE(frame.width(), 200);
        QCOMPARE(frame.x(), reservedArea.right() + 1 - 200);
        // the console and the frame have to tile the unreserved space between them
        QCOMPARE(mpHost->mpConsole->mpMainDisplay->geometry().right() + 1, frame.x());
    }

    // the console has to give up room for the frame on top of what the package took
    void test_frameBorderStacksOnTopOfTheUserBorder()
    {
        runLua(qsl("setBorderRight(300)"));
        settle();

        QVERIFY(createFrame(qsl("status"), qsl("right"), qsl("200px"), qsl("100%")));

        QCOMPARE(mpHost->userBorders().right(), 300);
        QCOMPARE(mpHost->borders().right(), 500);
        QVERIFY2(mpHost->mpConsole->mpMainDisplay->geometry().right() < frameGeometry(qsl("status")).left(), "the main display overlaps the frame");
    }

    // with nothing reserved a right frame still goes right up to the edge
    void test_rightFrameHugsTheEdgeWithoutAUserBorder()
    {
        QVERIFY(createFrame(qsl("status"), qsl("right"), qsl("200px"), qsl("100%")));

        QCOMPARE(frameGeometry(qsl("status")).right() + 1, area().width());
        QCOMPARE(mpHost->mpConsole->mpMainDisplay->geometry().right() + 1, frameGeometry(qsl("status")).x());
    }

    void test_frameFollowsABorderThatChangesAfterwards()
    {
        runLua(qsl("setBorderRight(300)"));
        settle();

        QVERIFY(createFrame(qsl("status"), qsl("right"), qsl("200px"), qsl("100%")));

        runLua(qsl("setBorderRight(100)"));
        settle();

        QCOMPARE(frameGeometry(qsl("status")).x(), area().right() + 1 - 200);
        QCOMPARE(mpHost->borders().right(), 300);

        // growing the reservation is the direction that would leave the frame
        // sitting inside it
        runLua(qsl("setBorderRight(400)"));
        settle();

        QCOMPARE(frameGeometry(qsl("status")).x(), area().right() + 1 - 200);
        QCOMPARE(mpHost->borders().right(), 600);
    }

    void test_frameFollowsAWindowResize()
    {
        runLua(qsl("setBorderRight(300)"));
        settle();

        QVERIFY(createFrame(qsl("status"), qsl("right"), qsl("200px"), qsl("100%")));
        const int widthBefore = mpHost->mpConsole->mpMainFrame->width();

        mudlet::self()->resize(1000, 700);
        settle();

        QVERIFY2(mpHost->mpConsole->mpMainFrame->width() != widthBefore, "the window did not actually resize");
        QCOMPARE(frameGeometry(qsl("status")).x(), area().right() + 1 - 200);
        // a container that moves without its text area following it would look
        // to the player like the frame did not move at all
        QVERIFY(frameConsole(qsl("status")));
        QCOMPARE(frameConsole(qsl("status"))->size(), frameWidget(qsl("status"))->size());
    }

    void test_leftFrameStartsAfterAReservedLeftBorder()
    {
        runLua(qsl("setBorderLeft(150)"));
        settle();

        QVERIFY(createFrame(qsl("nav"), qsl("left"), qsl("120px"), qsl("100%")));

        QCOMPARE(frameGeometry(qsl("nav")).x(), 150);
        QCOMPARE(mpHost->borders().left(), 270);
    }

    void test_topFrameStartsAfterAReservedTopBorder()
    {
        runLua(qsl("setBorderTop(150)"));
        settle();

        QVERIFY(createFrame(qsl("banner"), qsl("top"), qsl("100%"), qsl("60px")));

        const QRect frame = frameGeometry(qsl("banner"));
        QCOMPARE(frame.y(), 150);
        QCOMPARE(frame.height(), 60);
        QCOMPARE(mpHost->borders().top(), 210);
    }

    void test_bottomFrameKeepsClearOfAReservedBottomBorder()
    {
        runLua(qsl("setBorderBottom(120)"));
        settle();
        const QRect reservedArea = area();

        QVERIFY(createFrame(qsl("chat"), qsl("bottom"), qsl("100%"), qsl("80px")));

        const QRect frame = frameGeometry(qsl("chat"));
        QCOMPARE(frame.height(), 80);
        QCOMPARE(frame.y(), reservedArea.bottom() + 1 - 80);
        QCOMPARE(mpHost->borders().bottom(), 200);
        // an anchor that does not go through the same formula: the frame has to
        // clear the command line as well as the reserved strip
        QCOMPARE(frame.bottom() + 1, mpHost->mpConsole->height() - mpHost->mpConsole->mpCommandLine->height() - 120);
    }

    // WIDTH defaults to a percentage, which now resolves against the space the
    // package left rather than the whole window
    void test_percentageWidthResolvesAgainstTheUnreservedSpace()
    {
        runLua(qsl("setBorderRight(400)"));
        settle();
        const QRect reservedArea = area();

        QVERIFY(createFrame(qsl("status"), qsl("right"), QString(), qsl("100%")));

        QCOMPARE(frameGeometry(qsl("status")).width(), reservedArea.width() / 4);
    }

    // a frame opened while a DEST is active nests inside it and takes no space
    // from the main console
    void test_nestedFrameLeavesTheBordersAlone()
    {
        QVERIFY(createFrame(qsl("outer"), qsl("right"), qsl("300px"), qsl("100%")));
        const QMargins bordersWithOuter = mpHost->borders();

        mpHost->mMxpFrameManager.setDestination(qsl("outer"), false, false);
        QVERIFY(createFrame(qsl("nested"), qsl("top"), qsl("100%"), qsl("40px")));
        mpHost->mMxpFrameManager.clearDestination();

        QCOMPARE(mpHost->borders(), bordersWithOuter);
        QVERIFY2(frameGeometry(qsl("outer")).contains(frameGeometry(qsl("nested"))), "the nested frame is not inside its parent");

        // Relayouts have to be idempotent: a top-aligned nested frame sits at its
        // parent's top edge, and usedHeight accumulates, so without a reset each
        // pass would march it further down.
        for (int i = 0; i < 3; ++i) {
            runLua(qsl("setBorderLeft(%1)").arg(i * 10));
            settle();
            QCOMPARE(frameGeometry(qsl("nested")).y(), frameGeometry(qsl("outer")).y());
        }

        QCOMPARE(mpHost->borders().right(), bordersWithOuter.right());
    }

    // frames stack inwards, so the second one has to clear both the reserved
    // border and its neighbour
    void test_twoRightFramesStackInwardsFromTheReservedBorder()
    {
        runLua(qsl("setBorderRight(200)"));
        settle();
        const QRect reservedArea = area();

        QVERIFY(createFrame(qsl("outer"), qsl("right"), qsl("150px"), qsl("100%")));
        QVERIFY(createFrame(qsl("inner"), qsl("right"), qsl("100px"), qsl("100%")));

        QCOMPARE(frameGeometry(qsl("outer")).x(), reservedArea.right() + 1 - 150);
        QCOMPARE(frameGeometry(qsl("inner")).x(), reservedArea.right() + 1 - 150 - 100);
        QCOMPARE(mpHost->borders().right(), 450);
    }

    // an EXTERNAL frame lives in its own window: it neither takes space from the
    // main console nor may be dragged into main window coordinates by a relayout
    void test_externalFrameIsLeftAloneByARelayout()
    {
        QVERIFY(createFrame(qsl("popup"), qsl("left"), qsl("200px"), qsl("150px"), {{qsl("EXTERNAL"), qsl("true")}}));
        QWidget* popup = frameWidget(qsl("popup"));
        QVERIFY2(popup && popup->isWindow(), "the external frame is not a window of its own");
        const QRect geometryBefore = popup->geometry();
        QCOMPARE(mpHost->borders(), QMargins());

        mudlet::self()->resize(1000, 700);
        settle();

        QCOMPARE(mpHost->borders(), QMargins());
        QCOMPARE(popup->geometry(), geometryBefore);
    }

    // closing the outer frame has to pull the inner one back out to the edge
    void test_closingAFrameRepositionsTheRest()
    {
        runLua(qsl("setBorderRight(200)"));
        settle();
        const QRect reservedArea = area();

        QVERIFY(createFrame(qsl("outer"), qsl("right"), qsl("150px"), qsl("100%")));
        QVERIFY(createFrame(qsl("inner"), qsl("right"), qsl("100px"), qsl("100%")));

        QVERIFY(mpHost->mMxpFrameManager.closeFrame(qsl("outer")));
        settle();

        QCOMPARE(frameGeometry(qsl("inner")).x(), reservedArea.right() + 1 - 100);
        QCOMPARE(mpHost->borders().right(), 300);
    }

    // All three frame layouts get their console from
    // TMainConsole::createSubConsole(), parented inside the frame (never on
    // mpMainFrame, where createMiniConsole would flash it) and wired with the
    // same name and command-line focus proxies createMiniConsole gives out. A
    // titled frame takes the tabbed layout, an untitled one the borderless
    // layout, and DOCK plus ALIGN=client becomes a tab inside the titled one.
    void test_frameConsolesAreBuiltInsideTheirFrame()
    {
        QVERIFY(createFrame(qsl("titled"), qsl("right"), qsl("300px"), qsl("100%"), {{qsl("TITLE"), qsl("Titled")}}));
        QVERIFY(createFrame(qsl("plain"), qsl("left"), qsl("120px"), qsl("100%")));
        QVERIFY(createFrame(qsl("tab"), qsl("client"), qsl("100%"), qsl("100%"), {{qsl("DOCK"), qsl("titled")}}));
        QVERIFY2(frameTabs(qsl("titled")), "A titled frame should have taken the tabbed layout");

        QWidget* commandLine = mpHost->mpConsole->mpCommandLine;
        QVERIFY(commandLine);
        for (const QString& name : {qsl("titled"), qsl("plain"), qsl("tab")}) {
            QWidget* widget = frameWidget(name);
            TConsole* console = frameConsole(name);
            QVERIFY2(widget && console, qPrintable(qsl("Frame %1 should have a widget and a console").arg(name)));
            QVERIFY2(widget->isAncestorOf(console), qPrintable(qsl("The console of %1 should live inside its frame widget").arg(name)));
            QCOMPARE(console->objectName(), name);
            QCOMPARE(console->focusProxy(), commandLine);
            QCOMPARE(console->mUpperPane->focusProxy(), commandLine);
            QCOMPARE(console->mLowerPane->focusProxy(), commandLine);
        }
    }

    // A titled frame carries its title on the single tab of a header, an
    // untitled one has no header. Either way the console is shown, registered
    // under the frame's name and styled after the profile, a shade lighter so
    // that the frame stands out from the main window.
    void test_frameConsolesTakeTheProfilesLook()
    {
        // Colours and a font size a fresh console would not have by itself,
        // and not black, which lighter() leaves as it is
        TMainConsole* mainConsole = mpHost->mpConsole;
        const QColor savedFgColor = mainConsole->mFgColor;
        const QColor savedBgColor = mainConsole->mBgColor;
        const int savedFontSize = mpHost->getDisplayFont().pointSize();
        const auto restoreLook = qScopeGuard([&]() {
            mainConsole->mFgColor = savedFgColor;
            mainConsole->mBgColor = savedBgColor;
            mpHost->setDisplayFontSize(savedFontSize);
            settle();
        });
        mainConsole->mFgColor = QColor(200, 180, 160);
        mainConsole->mBgColor = QColor(40, 60, 80);
        mpHost->setDisplayFontSize(savedFontSize + 5);
        settle();

        QVERIFY(createFrame(qsl("titled"), qsl("right"), qsl("300px"), qsl("100%"), {{qsl("TITLE"), qsl("Status")}}));
        QVERIFY(createFrame(qsl("plain"), qsl("left"), qsl("120px"), qsl("100%")));

        QTabWidget* tabs = frameTabs(qsl("titled"));
        QVERIFY2(tabs, "A titled frame should have a tab header");
        QCOMPARE(tabs->count(), 1);
        QCOMPARE(tabs->tabText(0), qsl("Status"));
        QCOMPARE(tabs->currentIndex(), 0);
        QVERIFY2(!frameTabs(qsl("plain")), "An untitled frame should have no tab header");

        const int profileFontSize = savedFontSize + 5;
        for (const QString& name : {qsl("titled"), qsl("plain")}) {
            TConsole* console = frameConsole(name);
            QVERIFY(console);
            QVERIFY2(!console->isHidden(), qPrintable(qsl("The console of %1 should be shown").arg(name)));
            QVERIFY2(!frameWidget(name)->isHidden(), qPrintable(qsl("Frame %1 should be shown").arg(name)));
            QCOMPARE(mpHost->mpConsole->subConsoleWidget(name), console);
            QCOMPARE(console->mDisplayFontDetails.mPointSize, profileFontSize);
            // what the frame's text is printed in, until the game says otherwise
            QCOMPARE(console->model().mFormatCurrent.foreground(), QColor(200, 180, 160));
            QCOMPARE(console->model().mFormatCurrent.background(), QColor(40, 60, 80).lighter(115));
            QVERIFY(console->getScrolling());
        }
    }

    void test_scrollingNoTurnsScrollingOffInEveryLayout()
    {
        QVERIFY(createFrame(qsl("titled"), qsl("right"), qsl("300px"), qsl("100%"), {{qsl("SCROLLING"), qsl("NO")}, {qsl("TITLE"), qsl("Titled")}}));
        QVERIFY(createFrame(qsl("plain"), qsl("left"), qsl("120px"), qsl("100%"), {{qsl("SCROLLING"), qsl("NO")}}));
        QVERIFY(createFrame(qsl("tab"), qsl("client"), qsl("100%"), qsl("100%"), {{qsl("SCROLLING"), qsl("NO")}, {qsl("DOCK"), qsl("titled")}}));
        QVERIFY(createFrame(qsl("popup"), qsl("left"), qsl("200px"), qsl("150px"), {{qsl("SCROLLING"), qsl("NO")}, {qsl("EXTERNAL"), qsl("true")}}));

        for (const QString& name : {qsl("titled"), qsl("plain"), qsl("tab"), qsl("popup")}) {
            TConsole* console = frameConsole(name);
            QVERIFY2(console, qPrintable(qsl("Frame %1 should have a console").arg(name)));
            QVERIFY2(!console->getScrolling(), qPrintable(qsl("Frame %1 should not scroll").arg(name)));
        }
    }

    // DOCK plus ALIGN=client adds a tab to the named frame's header. The first
    // such tab is brought to the front, later ones are not.
    void test_tabFramesJoinTheirParentsHeader()
    {
        QVERIFY(createFrame(qsl("titled"), qsl("right"), qsl("300px"), qsl("100%"), {{qsl("TITLE"), qsl("Main")}}));
        QVERIFY(createFrame(qsl("chat"), qsl("client"), qsl("100%"), qsl("100%"), {{qsl("DOCK"), qsl("titled")}, {qsl("TITLE"), qsl("Chat")}}));

        QTabWidget* tabs = frameTabs(qsl("titled"));
        QVERIFY(tabs);
        QCOMPARE(tabs->count(), 2);
        QCOMPARE(tabs->tabText(1), qsl("Chat"));
        QCOMPARE(tabs->currentIndex(), 1);
        QCOMPARE(tabs->widget(1), frameWidget(qsl("chat")));
        QVERIFY(frameConsole(qsl("chat")));
        QCOMPARE(mpHost->mpConsole->subConsoleWidget(qsl("chat")), frameConsole(qsl("chat")));
        QVERIFY2(!frameConsole(qsl("chat"))->isHidden(), "The tab's console should be shown");

        QVERIFY(createFrame(qsl("log"), qsl("client"), qsl("100%"), qsl("100%"), {{qsl("DOCK"), qsl("titled")}}));
        QCOMPARE(tabs->count(), 3);
        QCOMPARE(tabs->tabText(2), qsl("log"));
        QCOMPARE(tabs->currentIndex(), 1);
    }

    // closing a tab takes only that tab out of its parent's header
    void test_closingATabFrameRemovesOnlyItsTab()
    {
        QVERIFY(createFrame(qsl("titled"), qsl("right"), qsl("300px"), qsl("100%"), {{qsl("TITLE"), qsl("Main")}}));
        QVERIFY(createFrame(qsl("chat"), qsl("client"), qsl("100%"), qsl("100%"), {{qsl("DOCK"), qsl("titled")}}));
        const QPointer<QWidget> chatPage = frameWidget(qsl("chat"));
        const QMargins bordersBefore = mpHost->borders();

        QVERIFY(mpHost->mMxpFrameManager.closeFrame(qsl("chat")));
        settle();

        QVERIFY(!mpHost->mMxpFrameManager.frameExists(qsl("chat")));
        QVERIFY2(chatPage.isNull(), "The closed tab's page should have been deleted");
        QVERIFY(!mpHost->mpConsole->subConsoleWidget(qsl("chat")));
        QVERIFY(!mpHost->windowRegistry().hasSubConsole(qsl("chat")));
        QCOMPARE(frameTabs(qsl("titled"))->count(), 1);
        QVERIFY(frameConsole(qsl("titled")));
        QCOMPARE(mpHost->borders(), bordersBefore);
    }

    // a DOCK into a frame without a header, or into no frame at all, falls
    // back to a frame of its own on the main window
    void test_dockWithoutATabbedParentFallsBackToAFrameOfItsOwn()
    {
        QVERIFY(createFrame(qsl("plain"), qsl("left"), qsl("120px"), qsl("100%")));
        QVERIFY(createFrame(qsl("orphan"), qsl("client"), qsl("200px"), qsl("100%"), {{qsl("DOCK"), qsl("plain")}}));
        QVERIFY(createFrame(qsl("lost"), qsl("client"), qsl("200px"), qsl("100%"), {{qsl("DOCK"), qsl("nowhere")}}));

        QVERIFY(!frameTabs(qsl("plain")));
        for (const QString& name : {qsl("orphan"), qsl("lost")}) {
            QVERIFY2(frameWidget(name), qPrintable(qsl("Frame %1 should have a widget").arg(name)));
            QCOMPARE(frameWidget(name)->parentWidget(), mpHost->mpConsole->mpMainFrame);
            QCOMPARE(frameWidget(name)->width(), 200);
            QVERIFY(frameConsole(name));
        }
    }

    void test_closingAFrameClosesTheFramesNestedInIt()
    {
        QVERIFY(createFrame(qsl("outer"), qsl("right"), qsl("300px"), qsl("100%")));
        mpHost->mMxpFrameManager.setDestination(qsl("outer"), false, false);
        QVERIFY(createFrame(qsl("nested"), qsl("top"), qsl("100%"), qsl("40px")));
        mpHost->mMxpFrameManager.clearDestination();
        const QPointer<QWidget> outer = frameWidget(qsl("outer"));
        const QPointer<QWidget> nested = frameWidget(qsl("nested"));
        QVERIFY(outer && nested);

        QVERIFY(mpHost->mMxpFrameManager.closeFrame(qsl("outer")));
        settle();

        QCOMPARE(mpHost->mMxpFrameManager.frameCount(), 0);
        QVERIFY(outer.isNull());
        QVERIFY(nested.isNull());
        QVERIFY(!mpHost->mpConsole->subConsoleWidget(qsl("outer")));
        QVERIFY(!mpHost->mpConsole->subConsoleWidget(qsl("nested")));
        QCOMPARE(mpHost->borders(), QMargins());
    }

    // ACTION=open on a frame that exists shows it again and brings it to the
    // front, leaving its size alone
    void test_openingAnExistingFrameShowsAndRaisesIt()
    {
        QVERIFY(createFrame(qsl("first"), qsl("right"), qsl("200px"), qsl("100%")));
        QVERIFY(createFrame(qsl("second"), qsl("right"), qsl("200px"), qsl("100%")));
        QWidget* first = frameWidget(qsl("first"));
        QWidget* second = frameWidget(qsl("second"));
        const QObjectList& siblings = mpHost->mpConsole->mpMainFrame->children();
        QVERIFY(siblings.indexOf(first) < siblings.indexOf(second));
        first->hide();

        QVERIFY(createFrame(qsl("first"), qsl("right"), qsl("50px"), qsl("100%")));

        QCOMPARE(mpHost->mMxpFrameManager.frameCount(), 2);
        QCOMPARE(frameWidget(qsl("first")), first);
        QVERIFY(!first->isHidden());
        QVERIFY2(siblings.indexOf(first) > siblings.indexOf(second), "The reopened frame should have been raised");
        QCOMPARE(first->width(), 200);
    }

    void test_focusActionRaisesAnExistingFrameOnly()
    {
        QVERIFY(createFrame(qsl("first"), qsl("right"), qsl("200px"), qsl("100%")));
        QVERIFY(createFrame(qsl("second"), qsl("right"), qsl("200px"), qsl("100%")));
        QWidget* first = frameWidget(qsl("first"));
        QWidget* second = frameWidget(qsl("second"));
        const QObjectList& siblings = mpHost->mpConsole->mpMainFrame->children();

        QVERIFY(createFrame(qsl("first"), QString(), QString(), QString(), {{qsl("ACTION"), qsl("focus")}}));
        QVERIFY2(siblings.indexOf(first) > siblings.indexOf(second), "The focused frame should have been raised");

        QVERIFY(!createFrame(qsl("missing"), QString(), QString(), QString(), {{qsl("ACTION"), qsl("focus")}}));
        QVERIFY(!mpHost->mMxpFrameManager.frameExists(qsl("missing")));
    }

    // an EXTERNAL frame is a titled window of its own, sized against the main console
    void test_externalFrameIsATitledWindowOfItsOwn()
    {
        const QSize consoleSize = mpHost->mpConsole->size();
        QVERIFY(createFrame(qsl("popup"), qsl("left"), qsl("50%"), qsl("25%"), {{qsl("EXTERNAL"), qsl("true")}, {qsl("TITLE"), qsl("Popup")}}));

        QWidget* popup = frameWidget(qsl("popup"));
        QVERIFY(popup);
        QVERIFY(popup->isWindow());
        QVERIFY(!popup->isHidden());
        QCOMPARE(popup->windowTitle(), qsl("Popup"));
        QCOMPARE(popup->size(), QSize(consoleSize.width() * 50 / 100, consoleSize.height() * 25 / 100));
        QCOMPARE(frameConsole(qsl("popup")), popup);
        QCOMPARE(mpHost->mpConsole->subConsoleWidget(qsl("popup")), frameConsole(qsl("popup")));
    }

    // DEST prints into the frame's own console, whatever the layout, and into
    // no frame once the redirect is over
    void test_destinationPrintsIntoTheFramesConsole()
    {
        QVERIFY(createFrame(qsl("plain"), qsl("left"), qsl("120px"), qsl("100%")));
        QVERIFY(createFrame(qsl("popup"), qsl("left"), qsl("200px"), qsl("150px"), {{qsl("EXTERNAL"), qsl("true")}}));
        auto& manager = mpHost->mMxpFrameManager;

        manager.setDestination(qsl("plain"), false, false);
        QCOMPARE(manager.currentDestinationSink(), static_cast<TPrintSink*>(frameConsole(qsl("plain"))));
        manager.setDestination(qsl("popup"), false, false);
        QCOMPARE(manager.currentDestinationSink(), static_cast<TPrintSink*>(frameConsole(qsl("popup"))));
        // an unknown frame leaves the redirect where it was
        manager.setDestination(qsl("nowhere"), false, false);
        QCOMPARE(manager.getCurrentDestination(), qsl("popup"));
        QCOMPARE(manager.currentDestinationSink(), static_cast<TPrintSink*>(frameConsole(qsl("popup"))));

        manager.clearDestination();
        QVERIFY(!manager.currentDestinationSink());
    }

    void test_resetClosesEveryFrame()
    {
        const QStringList names{qsl("titled"), qsl("tab"), qsl("plain"), qsl("popup")};
        QVERIFY(createFrame(qsl("titled"), qsl("right"), qsl("300px"), qsl("100%"), {{qsl("TITLE"), qsl("Main")}}));
        QVERIFY(createFrame(qsl("tab"), qsl("client"), qsl("100%"), qsl("100%"), {{qsl("DOCK"), qsl("titled")}}));
        QVERIFY(createFrame(qsl("plain"), qsl("left"), qsl("120px"), qsl("100%")));
        QVERIFY(createFrame(qsl("popup"), qsl("left"), qsl("200px"), qsl("150px"), {{qsl("EXTERNAL"), qsl("true")}}));
        QList<QPointer<QWidget>> widgets;
        for (const QString& name : names) {
            widgets << QPointer<QWidget>(frameWidget(name));
            QVERIFY(widgets.last());
        }
        mpHost->mMxpFrameManager.setDestination(qsl("plain"), false, false);

        mpHost->mMxpFrameManager.resetAllFrames();
        settle();

        QCOMPARE(mpHost->mMxpFrameManager.frameCount(), 0);
        QVERIFY(!mpHost->mMxpFrameManager.hasActiveDestination());
        QCOMPARE(mpHost->borders(), QMargins());
        for (const auto& widget : std::as_const(widgets)) {
            QVERIFY(widget.isNull());
        }
        for (const QString& name : names) {
            QVERIFY2(!mpHost->mpConsole->subConsoleWidget(name), qPrintable(qsl("%1 should no longer be registered").arg(name)));
            QVERIFY2(!mpHost->windowRegistry().hasSubConsole(name), qPrintable(qsl("%1 should no longer be in the window registry").arg(name)));
        }
    }

    // How the base UI reserves its space, so this is #9698 as reported. Declared
    // last on purpose: an adjustable container leaves deferred timers of its own
    // behind that resize the main window out from under whatever runs next, so
    // add new tests above this one rather than below it. For the same reason the
    // expectation is evaluated at assert time rather than captured up front.
    void test_rightFrameKeepsClearOfAnAttachedAdjustableContainer()
    {
        runLua(qsl("panel = Adjustable.Container:new({name = 'mxpTestPanel', x = '-25%', y = 0, width = '25%', height = '100%', autoSave = false, autoLoad = false})\n"
                   "panel:attachToBorder('right')"));
        settle();
        const int reservedRight = mpHost->userBorders().right();
        QVERIFY2(reservedRight > 0, "the adjustable container did not reserve a border");

        QVERIFY(createFrame(qsl("status"), qsl("right"), qsl("200px"), qsl("100%")));
        settle();

        QCOMPARE(frameGeometry(qsl("status")).x(), area().right() + 1 - 200);
        QCOMPARE(mpHost->borders().right(), reservedRight + 200);

        runLua(qsl("panel:detach() panel:hide()"));
        settle();
    }
};

#include "MxpFramePlacementTest.moc"
MUDLET_GROUPED_TEST_MAIN(MxpFramePlacementTest)
