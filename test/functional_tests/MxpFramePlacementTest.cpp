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

#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
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

    QRect frameGeometry(const QString& name) const
    {
        const TMxpFrame* frame = mpHost->mMxpFrameManager.getFrame(name);
        if (!frame || !frame->widget) {
            return {};
        }
        return frame->widget->geometry();
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

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
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
        const TMxpFrame* frame = mpHost->mMxpFrameManager.getFrame(qsl("status"));
        QVERIFY(frame && frame->console);
        QCOMPARE(frame->console->size(), frame->widget->size());
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
        const TMxpFrame* frame = mpHost->mMxpFrameManager.getFrame(qsl("popup"));
        QVERIFY(frame);
        QVERIFY2(frame->widget && frame->widget->isWindow(), "the external frame is not a window of its own");
        const QRect geometryBefore = frame->widget->geometry();
        QCOMPARE(mpHost->borders(), QMargins());

        mudlet::self()->resize(1000, 700);
        settle();

        QCOMPARE(mpHost->borders(), QMargins());
        QCOMPARE(frame->widget->geometry(), geometryBefore);
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
