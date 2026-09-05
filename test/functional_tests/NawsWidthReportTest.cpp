/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vperetokin@hey.com            *
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

#include <QPointer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <algorithm>
#include <chrono>

#include "PortableModeTestHelper.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TBuffer.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TTabBar.h"
#include "TelnetServerStub.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// What a game is told about the window it is drawing into. The cases that can
// read the wire do, through TelnetServerStub, rather than trusting the client's
// own idea of its size - only what reaches the game can wrap the game's output.
// The ones that assert on Host::mScreenWidth and getMainWindowSize() say so.
class NawsWidthReportTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    QPointer<Host> mpSecondHost;
    const QString mHostname = "NawsWidthFirst";
    const QString mSecondHostname = "NawsWidthSecond";
    QString mPort;
    const QString mLocalhost = "localhost";

    void settle(const std::chrono::milliseconds duration = 400ms)
    {
        QTest::qWait(duration);
        QCoreApplication::processEvents();
    }

    void runLua(Host* pHost, const QString& code) const { QVERIFY2(pHost->getLuaInterpreter()->compileAndExecuteScript(code), qPrintable(code)); }

    void showTab(const QString& hostname) const { mudlet::self()->mpTabBar->setCurrentIndex(mudlet::self()->mpTabBar->tabIndex(hostname)); }

    // What a correctly behaving Mudlet would put on the wire for this profile as
    // things stand - the console's settled width, less the timestamp gutter,
    // which is drawn outside the space the game gets to write into.
    int reportableWidth(const Host* pHost) const
    {
        const int gutter = pHost->mpConsole->showTimeStamps() ? TBuffer::smTimeStampFormat.size() : 0;
        return std::min(pHost->mScreenWidth, pHost->mWrapAt) - gutter;
    }

    static QSize screenSize(const Host* pHost) { return QSize(pHost->mScreenWidth, pHost->mScreenHeight); }

    // The wire carries min(columns, wrapAt) and the console never goes under 40
    // columns, so only a width strictly between the two can show a prediction
    // to be right or wrong
    static bool measurableWidth(const Host* pHost) { return pHost->mScreenWidth > 40 && pHost->mScreenWidth < pHost->mWrapAt; }

    // For a failure message: everything the game was told, in order
    static QByteArray describe(const QList<QSize>& updates)
    {
        QStringList parts;
        for (const QSize& update : updates) {
            parts << qsl("%1x%2").arg(update.width()).arg(update.height());
        }
        return qsl("NAWS updates: [%1]").arg(parts.join(qsl(", "))).toUtf8();
    }

    // Cases below run in declaration order but have to stand up on their own
    // too, since a single one can be named on the grouped binary's command line.
    // Reports rather than asserts: a QVERIFY here would return from this helper
    // and leave the case that called it running on a profile that is not there.
    bool ensureSecondProfile()
    {
        if (mpSecondHost) {
            return true;
        }
        if (!QDir().mkpath(mudlet::getMudletPath(enums::profileHomePath, mSecondHostname)) || !mudlet::self()->writeProfileData(mSecondHostname, qsl("url"), mLocalhost).first
            || !mudlet::self()->writeProfileData(mSecondHostname, qsl("port"), mPort).first) {
            return false;
        }
        // the second argument is offline, so this profile never opens a
        // connection of its own - the stub only ever sees the first profile
        runLua(mpHost, qsl("loadProfile('%1', true)").arg(mSecondHostname));
        for (int attempt = 0; attempt < 20 && mpSecondHost.isNull(); ++attempt) {
            settle(300ms);
            mpSecondHost = mudlet::self()->getHostManager().getHost(mSecondHostname);
        }
        settle(600ms);
        return !mpSecondHost.isNull();
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - refusing to touch a portable installation");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        mudlet::getQSettings()->setValue(qsl("uiTourShown"), true);
        mudlet::getQSettings()->sync();
        mudlet::self()->resize(1200, 800);

        QTimer::singleShot(0ms, qApp, [this]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100ms);
            QVERIFY(mudlet::self()->mpConnectionDialog);
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
        QSignalSpy profileLoaded(mudlet::self(), &mudlet::signal_profileLoaded);
        QVERIFY2(profileLoaded.wait(6000), "the first profile did not finish loading");
        mpHost = mudlet::self()->getActiveHost();
        QVERIFY(mpHost);

        // IAC DO NAWS, so the client starts reporting its window size at all
        mpServer->sendRaw(QByteArray("\xFF\xFD\x1F", 3));
        settle(600ms);
        // The game asked for the size during negotiation and is entitled to an
        // answer then, not once some later resize happens to shake one loose.
        // Without this every case below still passes if the reply never comes.
        QVERIFY2(!mpServer->nawsUpdates().isEmpty(), "the client never answered IAC DO NAWS");
    }

    // Leave the window, the gutter and the borders as the next case expects to
    // find them, whether this one passed or not - a QCOMPARE returns from the
    // function, so a restore written at the end of a case does not run when it
    // fails.
    void cleanup()
    {
        if (mpHost && mpHost->mpConsole && mpHost->mpConsole->showTimeStamps()) {
            mpHost->mpConsole->slot_toggleTimeStamps(false);
        }
        showTab(mHostname);
        if (mpHost) {
            mpHost->setUserBorders(QMargins());
        }
        mudlet::self()->resize(1200, 800);
        settle(600ms);
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // A resize is not one event: the console is laid out, and then any Geyser
    // container attached to a border re-reserves that border from a timer,
    // laying it out again at a different width. The game must hear the width
    // the window settles at, never one it passed through - whatever it is told
    // is what it wraps its next reply to.
    void resizeReportsOnlyTheWidthTheWindowSettlesAt()
    {
        runLua(mpHost,
               qsl("nawsProbeContainer = Adjustable.Container:new({name='nawsProbeContainer', x='-30%', y=0, width='30%', height='50%'})\n"
                   "nawsProbeContainer:attachToBorder('right')"));
        settle(800ms);
        QVERIFY2(mpHost->borders().right() > 0, "the container did not reserve a border, so there is no second layout step to wait out");

        mpServer->clearNawsUpdates();
        mudlet::self()->resize(800, 800);
        settle(1500ms);

        const auto narrowed = mpServer->nawsUpdates();
        QVERIFY2(narrowed.size() == 1, describe(narrowed).constData());
        QCOMPARE(narrowed.constFirst().width(), reportableWidth(mpHost));

        // and the debounce has to re-arm: a timer that only ever fires once
        // would leave the game wrapping to this width for the rest of the
        // session, which the single resize above cannot tell apart
        mpServer->clearNawsUpdates();
        mudlet::self()->resize(1100, 800);
        settle(1500ms);

        const auto widened = mpServer->nawsUpdates();
        QVERIFY2(widened.size() == 1, describe(widened).constData());
        QCOMPARE(widened.constFirst().width(), reportableWidth(mpHost));
        QVERIFY2(widened.constFirst().width() != narrowed.constFirst().width(), "the second resize reported the first one's width");
    }

    // The same resize with the timestamp gutter showing, which comes off every
    // width reported and so makes a mid-resize reading narrower still - narrow
    // enough for a game to wrap its output to about a third of the window.
    void resizeWithTimestampsShowingReportsOnlyTheSettledWidth()
    {
        mpHost->mpConsole->slot_toggleTimeStamps(true);
        settle(1200ms);

        mpServer->clearNawsUpdates();
        mudlet::self()->resize(800, 800);
        settle(1500ms);

        const auto updates = mpServer->nawsUpdates();
        QVERIFY2(updates.size() == 1, describe(updates).constData());
        QCOMPARE(updates.constFirst().width(), reportableWidth(mpHost));
    }

    // Showing the gutter is not a resize, but it does take columns away from
    // the game, so it has to be reported - and reported once.
    void showingTheTimestampGutterReportsTheColumnsItTakes()
    {
        QVERIFY(!mpHost->mpConsole->showTimeStamps());
        const int withoutGutter = reportableWidth(mpHost);

        mpServer->clearNawsUpdates();
        mpHost->mpConsole->slot_toggleTimeStamps(true);
        settle(1500ms);

        const auto updates = mpServer->nawsUpdates();
        QVERIFY2(updates.size() == 1, describe(updates).constData());
        QCOMPARE(updates.constFirst().width(), withoutGutter - TBuffer::smTimeStampFormat.size());
    }

    // A profile in a background tab has its own borders and its own font, so
    // the console on screen says nothing about how much room it has. Whatever
    // it is told while it waits has to be what it finds when it comes back:
    // anything else re-wraps the backlog the moment the user switches to it.
    void aBackgroundedProfileIsToldTheSizeItComesBackTo()
    {
        QVERIFY2(ensureSecondProfile(), "the second profile did not load");

        // The container from the first case re-reserves its border from a timer
        // of its own when this profile comes back, which would move the goalposts
        // between the two readings compared here; this case is about borders
        // that stay where they were put.
        runLua(mpHost, qsl("if nawsProbeContainer then nawsProbeContainer:detach() end"));
        // Borders on this profile only, so the two genuinely differ in how much
        // of the same window they have to write into
        runLua(mpHost, qsl("setBorderLeft(150) setBorderRight(150)"));
        showTab(mSecondHostname);
        settle(1500ms);
        QVERIFY2(mpHost->mpConsole->isHidden(), "the first profile should be in a background tab by now");

        const QSize before = screenSize(mpHost);
        mpServer->clearNawsUpdates();
        // narrower and shorter, so a stale row count shows up as well
        mudlet::self()->resize(1000, 600);
        settle(1500ms);

        const QSize toldWhileHidden = screenSize(mpHost);
        QVERIFY2(measurableWidth(mpHost), "at the 40 column floor or the wrap-at ceiling this case cannot tell a right prediction from a wrong one");
        QVERIFY2(toldWhileHidden.width() != mpSecondHost->mScreenWidth, "the two profiles have different borders, so they cannot both be this wide");
        QVERIFY2(toldWhileHidden.height() != before.height(), "the window lost height, but the hidden profile was never told a new row count");

        // and it has to have reached the game, not just the client's own idea
        // of itself - the profile is still connected while it sits in the tab
        const auto updates = mpServer->nawsUpdates();
        QVERIFY2(!updates.isEmpty(), "a backgrounded profile's corrected size never reached its game");
        QCOMPARE(updates.constLast(), QSize(reportableWidth(mpHost), toldWhileHidden.height()));

        showTab(mHostname);
        settle(1500ms);
        QCOMPARE(screenSize(mpHost), toldWhileHidden);
        // so coming back gave the game nothing new to re-wrap to
        QVERIFY2(mpServer->nawsUpdates().size() == updates.size(), describe(mpServer->nawsUpdates()).constData());
    }

    // A script can move its own profile's borders while that profile waits in a
    // background tab - a UI reacting to something the game sent, say. No resize
    // reaches a console that is zero pixels wide, so the game has to be told
    // from the border change itself, or it wraps to the old size until the
    // user switches back and it is corrected under them.
    void aBackgroundedProfileThatMovesItsBordersIsToldTheSizeItComesBackTo()
    {
        QVERIFY2(ensureSecondProfile(), "the second profile did not load");
        runLua(mpHost, qsl("if nawsProbeContainer then nawsProbeContainer:detach() end"));
        showTab(mSecondHostname);
        settle(1500ms);
        QVERIFY2(mpHost->mpConsole->isHidden(), "the first profile should be in a background tab by now");

        const QSize before = screenSize(mpHost);
        mpServer->clearNawsUpdates();
        // The window keeps its size here, so the side borders have to take the
        // width under the wrap-at ceiling on their own for the wire to show it
        runLua(mpHost, qsl("setBorderLeft(250) setBorderRight(250) setBorderTop(100)"));
        settle(1500ms);

        const auto updates = mpServer->nawsUpdates();
        QVERIFY2(!updates.isEmpty(), "the game was not told about borders moved while the profile was in the background");
        const QSize toldWhileHidden = screenSize(mpHost);
        QVERIFY2(measurableWidth(mpHost), "at the 40 column floor or the wrap-at ceiling this case cannot tell a right prediction from a wrong one");
        QVERIFY2(toldWhileHidden.height() < before.height(), "the top border took rows away, but the hidden profile was not told fewer");
        QCOMPARE(updates.constLast(), QSize(reportableWidth(mpHost), toldWhileHidden.height()));

        showTab(mHostname);
        settle(1500ms);
        QCOMPARE(screenSize(mpHost), toldWhileHidden);
    }

    // The game is told nothing at all for a height of zero, so borders that
    // leave the profile no rows must not cost it the width it would otherwise
    // have been told - that is still what its text wraps to.
    void aBackgroundedProfileLeftNoRowsIsStillToldItsWidth()
    {
        QVERIFY2(ensureSecondProfile(), "the second profile did not load");
        runLua(mpHost, qsl("if nawsProbeContainer then nawsProbeContainer:detach() end"));
        showTab(mSecondHostname);
        settle(1500ms);
        QVERIFY2(mpHost->mpConsole->isHidden(), "the first profile should be in a background tab by now");

        const int rowsBefore = mpHost->mScreenHeight;
        QVERIFY(rowsBefore > 0);
        mpServer->clearNawsUpdates();
        runLua(mpHost, qsl("setBorderLeft(250) setBorderRight(250) setBorderTop(%1)").arg(mpHost->mpConsole->parentWidget()->height()));
        settle(1500ms);

        const auto updates = mpServer->nawsUpdates();
        QVERIFY2(!updates.isEmpty(), "borders that left no rows cost the game the profile's new width");
        QVERIFY2(measurableWidth(mpHost), "at the 40 column floor or the wrap-at ceiling this case cannot tell a right prediction from a wrong one");
        QCOMPARE(updates.constLast(), QSize(reportableWidth(mpHost), rowsBefore));
    }

    // Geyser lays every element out against getMainWindowSize(), so a profile
    // that reads it while in a background tab has to get the window it will
    // come back to. Resizing while it is away is what separates that from the
    // size it last had on screen, which is all it used to be able to report.
    void aBackgroundedProfileReportsTheWindowItWillComeBackTo()
    {
        QVERIFY2(ensureSecondProfile(), "the second profile did not load");
        showTab(mSecondHostname);
        settle(1500ms);
        QVERIFY(mpHost->mpConsole->isHidden());

        mudlet::self()->resize(1000, 800);
        settle(1500ms);
        const QSize whileHidden = mpHost->mpConsole->getMainWindowSize();

        showTab(mHostname);
        settle(1500ms);
        const QSize onceBack = mpHost->mpConsole->getMainWindowSize();

        QCOMPARE(whileHidden, onceBack);
    }
};

#include "NawsWidthReportTest.moc"
MUDLET_GROUPED_TEST_MAIN(NawsWidthReportTest)
