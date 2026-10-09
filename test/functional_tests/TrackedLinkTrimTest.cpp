/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Makers                                   *
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
 * THyperlinkVisibilityManager addresses each concealed OSC 8 hyperlink by line
 * number, so a buffer trim invalidates every entry it holds unless it is told.
 * Revealing a link then writes into whichever line has taken that index - or,
 * where that line is too short to accept it, writes nothing and leaves the link
 * blanked out for good.
 *
 * The manager is driven directly rather than through a fed OSC 8 sequence: a
 * reveal is timer-driven, so an end-to-end version would rest on a delay
 * elapsing before a trim, which is how flaky tests start.
 */

#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest/QtTest>

#include <limits>

#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TConsole.h"
#include "THyperlinkVisibilityManager.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class TrackedLinkTrimTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("TrackedLinkTrim");
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mLinkText = qsl("SECRET");
    const QString mLinkCommand = qsl("go north");
    const QString mLinkHint = qsl("head north");
    static constexpr int csmLinesLimit = 100;
    static constexpr int csmBatchDeleteSize = 20;

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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        deleteProfileDirectory(mHostname);

        auto host = TestProfile::create(mHostname, mLocalhost, mPort);
        QVERIFY2(host, "no active host available for the test");
        QSignalSpy connectionSpy(&(host->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connectionSpy.wait(2s), "could not connect with the host");

        mpHost = HostManager::self()->getHost(mHostname);
        QVERIFY(mpHost);
        QVERIFY(mpHost->mainConsoleView());
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory(mHostname);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // The batch edge, in both directions. This drives adjustLineNumbers() rather
    // than a real trim on purpose: what is under test is its range check, and
    // reaching it through the buffer made the case depend on how many lines the
    // console happened to hold, which is not the same on every machine. That the
    // trim calls this at all is covered end to end by TBufferOSC_spec.lua.
    void test_adjustingLineNumbersDropsOnlyTheLinksOnRemovedLines()
    {
        auto& manager = mpHost->mainConsoleView()->getHyperlinkVisibilityManager();

        const int firstRemoved = 0;
        const int lastRemoved = csmBatchDeleteSize - 1;
        const int firstSurvivor = csmBatchDeleteSize;
        QVERIFY(manager.registerHyperlink(4, firstRemoved, 0, mLinkText.length(), mLinkText, concealedRevealStyling()));
        QVERIFY(manager.registerHyperlink(5, lastRemoved, 0, mLinkText.length(), mLinkText, concealedRevealStyling()));
        QVERIFY(manager.registerHyperlink(6, firstSurvivor, 0, mLinkText.length(), mLinkText, concealedRevealStyling()));

        manager.adjustLineNumbers(0, csmBatchDeleteSize);

        QVERIFY2(!manager.trackedLinkIds().contains(4), "the link on the first removed line was kept, so entries accumulate");
        QVERIFY2(!manager.trackedLinkIds().contains(5), "the link on the last removed line was kept, so entries accumulate");
        QVERIFY2(manager.trackedLinkIds().contains(6), "the link on the first surviving line was dropped, so it can never reveal");
        QCOMPARE(manager.mTrackedLinks[6].lineNumber, 0);
    }

    // Concealing a link zeroes its characters' indices, so the scan that decides
    // which links are still in use cannot see it - but it is still live.
    void test_concealedLinkKeepsItsCommandsAcrossATrim()
    {
        auto* pConsole = mpHost->mainConsoleView();
        auto& buffer = pConsole->buffer;
        auto& manager = pConsole->getHyperlinkVisibilityManager();
        QVERIFY(mpHost->clearWindow(qsl("main")));
        buffer.setBufferSize(csmLinesLimit, csmBatchDeleteSize);

        fill(pConsole, qsl("seed"), 45);
        const int linkId = appendLink(pConsole);
        qApp->processEvents();

        const int registeredOn = lineContaining(buffer, mLinkText);
        QVERIFY2(registeredOn >= 0, "the link text never reached the buffer");
        manager.registerHyperlink(linkId, registeredOn, 0, mLinkText.length(), mLinkText, concealLaterStyling());
        QVERIFY2(buffer.collectActiveLinkIds().contains(linkId), "the link was never visible to the scan, so concealing it proves nothing");

        manager.concealLink(linkId);
        QVERIFY2(manager.isLinkConcealed(linkId), "the link did not conceal");
        QVERIFY2(!buffer.collectActiveLinkIds().contains(linkId), "the scan can still see the concealed link, so this test proves nothing");

        fill(pConsole, qsl("filler"), 60);
        qApp->processEvents();

        QVERIFY2(lineContaining(buffer, qsl("seed line 0")) < 0, "the buffer did not trim, so this test proves nothing");
        QCOMPARE(pConsole->getLinkStore().getLinksConst(linkId), QStringList{mLinkCommand});
        QCOMPARE(pConsole->getLinkStore().getHintsConst(linkId), QStringList{mLinkHint});
    }

    // Trims sweep a quarter of the buffer apart, so the link that last had a reused
    // id may have been trimmed without being swept yet
    void test_aRecycledLinkIdStartsWithoutTheTrimmedLinksState()
    {
        auto* pConsole = mpHost->mpConsole.data();
        auto& buffer = pConsole->buffer;
        const auto restoreStore = qScopeGuard([this, &buffer] {
            mpHost->clearWindow(qsl("main"));
            buffer.mLinkStore = TLinkStore();
        });

        // Which trim sweeps depends on how far the buffer filled before the link
        // went in, so try one batch either way and use whichever left it unswept
        bool trimmedUnswept = false;
        int oldId = 0;
        for (const int offset : {0, csmBatchDeleteSize}) {
            QVERIFY(mpHost->clearWindow(qsl("main")));
            buffer.mLinkStore = TLinkStore(3);
            buffer.setBufferSize(csmLinesLimit, csmBatchDeleteSize);
            fill(pConsole, qsl("seed"), csmLinesLimit + 1 + offset);
            oldId = appendLink(pConsole);
            buffer.markLinkAsVisited(oldId);
            for (int i = 0; i < 2 * csmLinesLimit && lineContaining(buffer, mLinkText) >= 0; ++i) {
                pConsole->print(qsl("filler line %1\n").arg(i));
            }
            QVERIFY(lineContaining(buffer, mLinkText) < 0);
            if (buffer.isLinkVisited(oldId)) {
                trimmedUnswept = true;
                break;
            }
        }
        QVERIFY2(trimmedUnswept, "every trim that removed the link swept it, so this test proves nothing");

        int newId = 0;
        for (int i = 0; i < 3; ++i) {
            newId = appendLink(pConsole);
        }
        QCOMPARE(newId, oldId);
        QVERIFY2(!buffer.isLinkVisited(newId), "a recycled id inherited the trimmed link's visited state");
    }

    // A link can leave the screen without its line going: replaced by a trigger,
    // covered by another link, or cleared with the line it is on
    void test_aLinkRemovedFromItsLineLeavesNoStateForItsId_data()
    {
        QTest::addColumn<int>("removal");
        QTest::newRow("replaced") << 0;
        QTest::newRow("covered by another link") << 1;
        QTest::newRow("line cleared") << 2;
    }

    void test_aLinkRemovedFromItsLineLeavesNoStateForItsId()
    {
        QFETCH(int, removal);
        auto* pConsole = mpHost->mpConsole.data();
        auto& buffer = pConsole->buffer;
        QVERIFY(mpHost->clearWindow(qsl("main")));
        buffer.mLinkStore = TLinkStore(3);
        const auto restoreStore = qScopeGuard([this, &buffer] {
            mpHost->clearWindow(qsl("main"));
            buffer.mLinkStore = TLinkStore();
        });

        QStringList commands{mLinkCommand};
        QStringList hints{mLinkHint};
        TChar format;
        buffer.addLink(false, mLinkText, commands, hints, format);
        const int oldId = pConsole->getLinkStore().getCurrentLinkID();
        buffer.markLinkAsVisited(oldId);
        const int line = buffer.getLastLineNumber();
        QPoint start(0, line);
        QPoint end(mLinkText.size(), line);
        switch (removal) {
        case 0:
            QVERIFY(buffer.replaceInLine(start, end, qsl("gone"), format));
            break;
        case 1:
            QVERIFY(buffer.applyLink(start, end, commands, hints));
            break;
        default:
            buffer.clearLastLine();
        }
        pConsole->print(qsl("\n"));
        QVERIFY(lineContaining(buffer, mLinkText) < 0 || buffer.getLinkIndexAt(line, 0) != oldId);

        int newId = 0;
        for (int i = 0; i < 3 && newId != oldId; ++i) {
            newId = appendLink(pConsole);
        }
        QCOMPARE(newId, oldId);
        QVERIFY2(!buffer.isLinkVisited(newId), "a recycled id inherited the removed link's visited state");
    }

    // When the newest link with an id goes early, or never reaches the screen, the
    // older one still showing must leave its state behind when it is trimmed
    void test_anOlderLinkTrimmedAfterItsIdWasHandedOutAgainLeavesNoState_data()
    {
        QTest::addColumn<bool>("newerLinkShown");
        QTest::newRow("newer link gagged") << true;
        QTest::newRow("newer link without text") << false;
    }

    void test_anOlderLinkTrimmedAfterItsIdWasHandedOutAgainLeavesNoState()
    {
        QFETCH(bool, newerLinkShown);
        auto* pConsole = mpHost->mpConsole.data();
        auto& buffer = pConsole->buffer;
        QVERIFY(mpHost->clearWindow(qsl("main")));
        buffer.mLinkStore = TLinkStore(3);
        buffer.setBufferSize(csmLinesLimit, csmBatchDeleteSize);
        const auto restoreStore = qScopeGuard([this, &buffer] {
            mpHost->clearWindow(qsl("main"));
            buffer.mLinkStore = TLinkStore();
        });

        const int oldId = appendLink(pConsole);
        buffer.markLinkAsVisited(oldId);
        for (int i = 0; i < 2; ++i) {
            appendLink(pConsole);
        }
        if (newerLinkShown) {
            QCOMPARE(appendLink(pConsole), oldId);
            const int newerLine = buffer.getLastLineNumber() - 1;
            QCOMPARE(buffer.getLinkIndexAt(newerLine, 0), oldId);
            QVERIFY(buffer.deleteLine(newerLine));
        } else {
            QStringList commands{mLinkCommand};
            QStringList hints{mLinkHint};
            buffer.addLink(false, QString(), commands, hints, TChar());
            QCOMPARE(pConsole->getLinkStore().getCurrentLinkID(), oldId);
        }

        // A sweep while the older link still shows keeps its state
        buffer.clearLinkState();
        QVERIFY(buffer.isLinkVisited(oldId));

        buffer.mLinesUntilLinkSweep = std::numeric_limits<int>::max();
        for (int i = 0; i < 2 * csmLinesLimit && lineContaining(buffer, mLinkText) >= 0; ++i) {
            pConsole->print(qsl("filler line %1\n").arg(i));
        }
        QVERIFY(lineContaining(buffer, mLinkText) < 0);

        int newId = 0;
        for (int i = 0; i < 3 && newId != oldId; ++i) {
            newId = appendLink(pConsole);
        }
        QCOMPARE(newId, oldId);
        QVERIFY2(!buffer.isLinkVisited(newId), "a recycled id inherited the trimmed link's visited state");
    }

    // Enter runs the focused id, which must not outlive its link when that id was
    // handed out again to a link that never put any text in the buffer
    void test_aFocusedLinkTrimmedAfterItsIdWentToAnEmptyLinkLosesFocus()
    {
        auto* pConsole = mpHost->mpConsole.data();
        auto& buffer = pConsole->buffer;
        QVERIFY(mpHost->clearWindow(qsl("main")));
        buffer.mLinkStore = TLinkStore(3);
        buffer.setBufferSize(csmLinesLimit, csmBatchDeleteSize);
        const auto restoreStore = qScopeGuard([this, &buffer] {
            mpHost->clearWindow(qsl("main"));
            buffer.mLinkStore = TLinkStore();
        });

        const int oldId = appendLink(pConsole);
        for (int i = 0; i < 2; ++i) {
            appendLink(pConsole);
        }
        buffer.clearLinkState();
        buffer.setFocusedLink(oldId);
        QStringList commands{mLinkCommand};
        QStringList hints{mLinkHint};
        buffer.addLink(false, QString(), commands, hints, TChar());
        QCOMPARE(pConsole->getLinkStore().getCurrentLinkID(), oldId);

        buffer.mLinesUntilLinkSweep = std::numeric_limits<int>::max();
        for (int i = 0; i < 2 * csmLinesLimit && lineContaining(buffer, mLinkText) >= 0; ++i) {
            pConsole->print(qsl("filler line %1\n").arg(i));
        }
        QVERIFY(lineContaining(buffer, mLinkText) < 0);
        QCOMPARE(buffer.getFocusedLink(), 0);
    }

    // Once every id has been handed out, a link-dense buffer trims only links whose
    // ids already belong to newer ones further down, and recording those would make
    // the next link added rescan the whole buffer on every trim
    void test_trimmingLinksWhoseIdsWereHandedOutAgainRecordsNone()
    {
        auto* pConsole = mpHost->mpConsole.data();
        auto& buffer = pConsole->buffer;
        QVERIFY(mpHost->clearWindow(qsl("main")));
        buffer.mLinkStore = TLinkStore(10);
        buffer.setBufferSize(csmLinesLimit, csmBatchDeleteSize);
        const auto restoreStore = qScopeGuard([this, &buffer] {
            mpHost->clearWindow(qsl("main"));
            buffer.mLinkStore = TLinkStore();
        });

        for (int i = 0; i < 4 * csmLinesLimit; ++i) {
            appendLink(pConsole);
            QVERIFY2(buffer.mLinkIdsRemovedSinceSweep.isEmpty(), qPrintable(qsl("link %1 left the trimmed ids recorded").arg(i)));
        }
        // the last trim removed links rather than lines without any
        QVERIFY(buffer.getLinkIndexAt(0, 0) > 0);
    }

    // A link still on screen when its id comes round again shares that id with the
    // new one, so each spoiler must reveal its own text where it is
    void test_spoilersSharingARecycledIdEachRevealTheirOwnText()
    {
        auto* pConsole = mpHost->mpConsole.data();
        auto& buffer = pConsole->buffer;
        QVERIFY(mpHost->clearWindow(qsl("main")));
        buffer.mLinkStore = TLinkStore(3);
        const auto restoreStore = qScopeGuard([this, &buffer] {
            mpHost->clearWindow(qsl("main"));
            buffer.mLinkStore = TLinkStore();
        });
        const auto printSpoiler = [this](const QString& text) {
            std::string data = qsl("\x1b]8;;send:reveal?config={\"spoiler\":true}\x1b\\%1\x1b]8;;\x1b\\\n").arg(text).toStdString();
            mpHost->printOnDisplay(data, true);
        };

        const auto lastLinkLine = [&buffer] {
            for (int i = buffer.getLastLineNumber(); i >= 0; --i) {
                if (buffer.getLinkIndexAt(i, 0) > 0) {
                    return i;
                }
            }
            return -1;
        };

        printSpoiler(qsl("OLDER"));
        const int olderLine = lastLinkLine();
        QVERIFY(olderLine >= 0);
        const int oldId = buffer.getLinkIndexAt(olderLine, 0);
        QVERIFY2(buffer.isSpoilerUnrevealed(oldId), "the first spoiler was not masked");

        for (int i = 0; i < 2; ++i) {
            appendLink(pConsole);
        }
        printSpoiler(qsl("NEWER"));
        const int newerLine = lastLinkLine();
        QVERIFY(newerLine > olderLine);
        QCOMPARE(buffer.getLinkIndexAt(newerLine, 0), oldId);
        QCOMPARE(buffer.getLinkIndexAt(olderLine, 0), oldId);

        buffer.revealSpoilerLink(oldId);
        QCOMPARE(buffer.line(olderLine).trimmed(), qsl("OLDER"));
        QCOMPARE(buffer.line(newerLine).trimmed(), qsl("NEWER"));
    }

    // Enter runs the focused link, so it has to go with its line on the trim that
    // removes it, not at the next scheduled sweep
    void test_aFocusedLinkLeavesTheStoreWithItsLine_data()
    {
        QTest::addColumn<bool>("conceal");
        QTest::newRow("shown") << false;
        QTest::newRow("concealed") << true;
    }

    void test_aFocusedLinkLeavesTheStoreWithItsLine()
    {
        QFETCH(bool, conceal);
        bool unfocusedOutlivedItsLine = false;
        QVERIFY(trimLinkAway(false, conceal, unfocusedOutlivedItsLine));
        QVERIFY2(unfocusedOutlivedItsLine, "the trim that removed the link swept the store anyway, so this test proves nothing");

        bool focusedOutlivedItsLine = true;
        QVERIFY(trimLinkAway(true, conceal, focusedOutlivedItsLine));
        QVERIFY2(!focusedOutlivedItsLine, "a focused link outlived its line in the store");
        QCOMPARE(mpHost->mpConsole->buffer.getFocusedLink(), 0);
    }

    // A wholeline concealment deletes the link's own line, so every link below
    // it moves up exactly one.
    void test_aWholelineConcealmentMovesTheLinksBelowItOnlyOnce()
    {
        auto* pConsole = mpHost->mainConsoleView();
        auto& buffer = pConsole->buffer;
        auto& manager = pConsole->getHyperlinkVisibilityManager();
        QVERIFY(mpHost->clearWindow(qsl("main")));

        fill(pConsole, qsl("seed"), 3);
        const int goingId = appendLink(pConsole);
        fill(pConsole, qsl("between"), 2);
        const int survivingId = appendLink(pConsole);
        qApp->processEvents();

        const int goingOn = lineContaining(buffer, mLinkText);
        QVERIFY2(goingOn >= 0, "the first link's text never reached the buffer");
        const int survivingOn = lineContaining(buffer, mLinkText, goingOn + 1);
        QVERIFY2(survivingOn > goingOn, "the second link has to sit below the first for this to cover anything");

        // returns whether the text should be blanked, and this one starts visible
        manager.registerHyperlink(goingId, goingOn, 0, mLinkText.length(), mLinkText, wholeLineConcealStyling());
        manager.registerHyperlink(survivingId, survivingOn, 0, mLinkText.length(), mLinkText, concealLaterStyling());

        // blank the survivor, so that the reveal below is the only thing that can
        // put its text back - otherwise the text sits there and the case passes
        // however wrong the line number has become
        manager.concealLink(survivingId);
        QVERIFY2(manager.isLinkConcealed(survivingId), "the surviving link did not conceal");
        QVERIFY2(lineContaining(buffer, mLinkText) == goingOn, "the survivor's text is still in the buffer, so a reveal would prove nothing");

        // clicking conceals it, and a wholeline concealment takes the line with it
        manager.onLinkClicked(goingId);
        qApp->processEvents();
        QVERIFY2(!manager.trackedLinkIds().contains(goingId), "the clicked link was not concealed away");
        QVERIFY2(manager.trackedLinkIds().contains(survivingId), "the link below was dropped along with the deleted line");
        QVERIFY2(lineContaining(buffer, mLinkText) < 0, "the deleted line took its text but the survivor's came back early");

        // revealing writes the link's text back at the line number the manager
        // holds, so where it lands is what that number now says
        manager.revealLink(survivingId);
        qApp->processEvents();
        QCOMPARE(lineContaining(buffer, mLinkText), survivingOn - 1);
    }

    // Clearing the window deletes every line, so nothing is referenced any more
    // and a tracked link's command has to go with the rest.
    void test_clearingTheWindowStillDropsATrackedLinksCommands()
    {
        auto* pConsole = mpHost->mainConsoleView();
        auto& manager = pConsole->getHyperlinkVisibilityManager();

        fill(pConsole, qsl("seed"), 5);
        const int linkId = appendLink(pConsole);
        qApp->processEvents();

        const int registeredOn = lineContaining(pConsole->buffer, mLinkText);
        QVERIFY(registeredOn >= 0);
        QVERIFY(manager.registerHyperlink(linkId, registeredOn, 0, mLinkText.length(), mLinkText, concealedRevealStyling()));
        QCOMPARE(pConsole->getLinkStore().getLinksConst(linkId), QStringList{mLinkCommand});

        QVERIFY(mpHost->clearWindow(qsl("main")));
        qApp->processEvents();

        QVERIFY2(pConsole->getLinkStore().getLinksConst(linkId).isEmpty(), "clearing the window left the link's command behind");
    }

    // A concealment queues a screen-reader announcement 300ms out, and clearing
    // the window in between leaves it naming links the reader can no longer
    // reach, so it has to be dropped along with them.
    void test_clearingTheWindowDropsAQueuedHiddenAnnouncement()
    {
        auto* pConsole = mpHost->mainConsoleView();
        auto& manager = pConsole->getHyperlinkVisibilityManager();
        QVERIFY(mpHost->clearWindow(qsl("main")));

        fill(pConsole, qsl("seed"), 3);
        const int linkId = appendLink(pConsole);
        qApp->processEvents();

        const int registeredOn = lineContaining(pConsole->buffer, mLinkText);
        QVERIFY(registeredOn >= 0);
        manager.registerHyperlink(linkId, registeredOn, 0, mLinkText.length(), mLinkText, concealLaterStyling());

        manager.concealLink(linkId);
        QVERIFY2(manager.mPendingHiddenCount > 0, "nothing was queued, so clearing it below proves nothing");
        QVERIFY2(manager.mpAnnouncementTimer->isActive(), "nothing was queued, so clearing it below proves nothing");

        QVERIFY(mpHost->clearWindow(qsl("main")));

        QCOMPARE(manager.mPendingHiddenCount, 0);
        QVERIFY2(!manager.mpAnnouncementTimer->isActive(), "the announcement still fires after the links it counts have gone");
    }

    // The main console's model outlives the view built on it and keeps taking
    // lines meanwhile, so the buffer moves the tracked links itself rather than
    // leaving it to the view.
    void test_deletingALineMovesTrackedLinks()
    {
        auto* pConsole = mpHost->mainConsoleView();
        auto& buffer = pConsole->buffer;
        auto& manager = pConsole->getHyperlinkVisibilityManager();
        QVERIFY(mpHost->clearWindow(qsl("main")));

        fill(pConsole, qsl("seed"), 3);
        const int linkId = appendLink(pConsole);
        fill(pConsole, qsl("trailing"), 3);
        qApp->processEvents();

        const int registeredOn = lineContaining(buffer, mLinkText);
        QVERIFY2(registeredOn > 0, "the link has to sit below the line deleted here");
        manager.registerHyperlink(linkId, registeredOn, 0, mLinkText.length(), mLinkText, concealLaterStyling());

        // blanks the link's text, so the reveal below is the only thing that can
        // put it back and where it lands is what the stored line number says
        manager.concealLink(linkId);
        QVERIFY2(lineContaining(buffer, mLinkText) < 0, "the link's text is still in the buffer, so a reveal would prove nothing");

        buffer.deleteLine(0);

        manager.revealLink(linkId);
        qApp->processEvents();
        QCOMPARE(lineContaining(buffer, mLinkText), registeredOn - 1);
    }

    // A buffer clears itself as it is built, and every buffer of the host that
    // is not the one a manager tracks has to leave that manager alone - a copy
    // or a cut would otherwise drop every link on the console it was taken from.
    void test_anotherBuffersLifecycleLeavesTheTrackedLinksAlone()
    {
        auto* pConsole = mpHost->mainConsoleView();
        auto& manager = pConsole->getHyperlinkVisibilityManager();
        QVERIFY(mpHost->clearWindow(qsl("main")));

        fill(pConsole, qsl("seed"), 3);
        const int linkId = appendLink(pConsole);
        qApp->processEvents();

        const int registeredOn = lineContaining(pConsole->buffer, mLinkText);
        QVERIFY(registeredOn >= 0);
        QVERIFY(manager.registerHyperlink(linkId, registeredOn, 0, mLinkText.length(), mLinkText, concealedRevealStyling()));

        // what a copy or a cut builds
        TBuffer viewlessSlice(mpHost);
        QVERIFY2(manager.trackedLinkIds().contains(linkId), "building a viewless buffer dropped another buffer's links");

        viewlessSlice.deleteLine(0);
        QCOMPARE(manager.mTrackedLinks[linkId].lineNumber, registeredOn);
    }

    // Every console has its own model and its own manager, so a miniconsole has
    // to maintain its links off its own buffer rather than the main one's.
    void test_aMiniconsoleMaintainsItsOwnTrackedLinks()
    {
        auto* pMain = mpHost->mainConsoleView();
        auto* pMini = pMain->createMiniConsole(QString(), qsl("trackedLinkMini"), 0, 0, 300, 100);
        QVERIFY2(pMini, "the miniconsole was not created, so this test proves nothing");
        QVERIFY2(&pMini->getHyperlinkVisibilityManager() != &pMain->getHyperlinkVisibilityManager(), "the miniconsole shares the main console's manager");

        auto& buffer = pMini->buffer;
        auto& manager = pMini->getHyperlinkVisibilityManager();

        fill(pMini, qsl("seed"), 3);
        const int linkId = appendLink(pMini);
        qApp->processEvents();

        const int registeredOn = lineContaining(buffer, mLinkText);
        QVERIFY2(registeredOn > 0, "the link has to sit below the line deleted here");
        QVERIFY(manager.registerHyperlink(linkId, registeredOn, 0, mLinkText.length(), mLinkText, concealedRevealStyling()));

        buffer.deleteLine(0);

        QCOMPARE(manager.mTrackedLinks[linkId].lineNumber, registeredOn - 1);
    }

private:
    // A reveal needs a delay or an expire trigger, otherwise registerHyperlink()
    // refuses to conceal it - nothing would ever reveal it again. Its 100ms poll
    // still runs; the delay is long enough that it never reaches the reveal.
    Mudlet::HyperlinkStyling concealedRevealStyling() const
    {
        Mudlet::HyperlinkStyling styling;
        styling.visibility.hasVisibilitySettings = true;
        styling.visibility.action = Mudlet::HyperlinkStyling::VisibilitySettings::Action::Reveal;
        styling.visibility.isConcealed = true;
        styling.visibility.delayMs = 600000;
        return styling;
    }

    int appendLink(TConsole* pConsole) const
    {
        QStringList commands{mLinkCommand};
        QStringList hints{mLinkHint};
        const TChar& current = pConsole->mFormatCurrent;
        TChar format(current.foreground(), current.background(), current.allDisplayAttributes());
        pConsole->buffer.addLink(false, mLinkText, commands, hints, format);
        pConsole->print(qsl("\n"));
        return pConsole->getLinkStore().getCurrentLinkID();
    }

    // Registers visible, so concealLink() below does the concealing and zeroes
    // the character indices. The long delay keeps the poll off the conceal.
    Mudlet::HyperlinkStyling concealLaterStyling() const
    {
        Mudlet::HyperlinkStyling styling;
        styling.visibility.hasVisibilitySettings = true;
        styling.visibility.action = Mudlet::HyperlinkStyling::VisibilitySettings::Action::Conceal;
        styling.visibility.isConcealed = false;
        styling.visibility.delayMs = 600000;
        return styling;
    }

    // Fills the buffer past one trim, appends a link and prints until its line is
    // trimmed away, reporting whether the store still holds the link at that point
    bool trimLinkAway(const bool focus, const bool conceal, bool& outlivedItsLine) const
    {
        auto* pConsole = mpHost->mpConsole.data();
        auto& buffer = pConsole->buffer;
        auto& manager = pConsole->getHyperlinkVisibilityManager();
        if (!mpHost->clearWindow(qsl("main"))) {
            return false;
        }
        buffer.setBufferSize(csmLinesLimit, csmBatchDeleteSize);
        fill(pConsole, qsl("seed"), csmLinesLimit + 1);
        const int linkId = appendLink(pConsole);
        if (focus) {
            buffer.setFocusedLink(linkId);
        }
        if (conceal) {
            manager.registerHyperlink(linkId, lineContaining(buffer, mLinkText), 0, mLinkText.length(), mLinkText, concealLaterStyling());
            manager.concealLink(linkId);
            if (buffer.collectActiveLinkIds().contains(linkId)) {
                return false;
            }
        }
        // a concealed link's line is found through its tracking, as its characters carry no index
        const auto onItsLine = [&] {
            return conceal ? manager.trackedLinkIds().contains(linkId) : lineContaining(buffer, mLinkText) >= 0;
        };
        for (int i = 0; i < 2 * csmLinesLimit && onItsLine(); ++i) {
            pConsole->print(qsl("filler line %1\n").arg(i));
        }
        if (onItsLine()) {
            return false;
        }
        outlivedItsLine = !pConsole->getLinkStore().getLinksConst(linkId).isEmpty();
        return true;
    }

    void fill(TConsole* pConsole, const QString& tag, const int lines) const
    {
        for (int i = 0; i < lines; ++i) {
            pConsole->print(qsl("%1 line %2\n").arg(tag).arg(i));
        }
    }

    // The wholeline flag is what makes performConcealment() delete the line
    // rather than blank the link out. No delay and no expire trigger, so the
    // click conceals it there and then.
    Mudlet::HyperlinkStyling wholeLineConcealStyling() const
    {
        Mudlet::HyperlinkStyling styling;
        styling.visibility.hasVisibilitySettings = true;
        styling.visibility.action = Mudlet::HyperlinkStyling::VisibilitySettings::Action::Conceal;
        styling.visibility.isConcealed = false;
        styling.visibility.deletesEntireLine = true;
        return styling;
    }

    int lineContaining(const TBuffer& buffer, const QString& needle, const int from = 0) const
    {
        for (int i = from; i < static_cast<int>(buffer.lineBuffer.size()); ++i) {
            if (buffer.lineBuffer.at(i).contains(needle)) {
                return i;
            }
        }
        return -1;
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "TrackedLinkTrimTest.moc"
MUDLET_GROUPED_TEST_MAIN(TrackedLinkTrimTest)
