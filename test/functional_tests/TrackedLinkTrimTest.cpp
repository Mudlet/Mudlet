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

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

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
        QVERIFY2(connectionSpy.wait(2000), "could not connect with the host");

        mpHost = mudlet::self()->getHostManager().getHost(mHostname);
        QVERIFY(mpHost);
        QVERIFY(mpHost->mpConsole);
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

    // The last line a trim takes is the batch edge, so a link there has to be
    // forgotten rather than left pointing into the buffer.
    void test_trimForgetsTheLinkOnTheLastCasualtyLine()
    {
        auto* pConsole = mpHost->mpConsole.data();
        auto& manager = pConsole->getHyperlinkVisibilityManager();

        const int lastCasualty = csmBatchDeleteSize - 1;
        QVERIFY(fillToTheBrink(pConsole, lastCasualty));
        QVERIFY(manager.registerHyperlink(3, lastCasualty, 0, mLinkText.length(), mLinkText, concealedRevealStyling()));

        tipItOver(pConsole);

        QVERIFY2(!manager.isLinkConcealed(3), "the link on the last trimmed line is still tracked, so entries accumulate");
    }

    // The line straight after that edge is the first survivor, and it becomes
    // line 0 - the off-by-one that drops it instead is the "never reveals" bug.
    void test_trimMovesTheLinkOnTheFirstSurvivingLineToTheTop()
    {
        auto* pConsole = mpHost->mpConsole.data();
        auto& buffer = pConsole->buffer;
        auto& manager = pConsole->getHyperlinkVisibilityManager();

        const int firstSurvivor = csmBatchDeleteSize;
        QVERIFY(fillToTheBrink(pConsole, firstSurvivor));
        QVERIFY(manager.registerHyperlink(4, firstSurvivor, 0, mLinkText.length(), mLinkText, concealedRevealStyling()));

        tipItOver(pConsole);

        manager.revealLink(4);
        qApp->processEvents();

        QVERIFY2(buffer.lineBuffer.at(0).startsWith(mLinkText), qPrintable(qsl("the first surviving line should have become line 0, which now reads \"%1\"").arg(buffer.lineBuffer.at(0))));

        // the text alone is not a working link, and no Lua call can read this back
        for (int column = 0; column < mLinkText.length(); ++column) {
            QCOMPARE(buffer.buffer.at(0).at(column).linkIndex(), 4);
        }
    }

    // Concealing a link zeroes its characters' indices, so the scan that decides
    // which links are still in use cannot see it - but it is still live.
    void test_concealedLinkKeepsItsCommandsAcrossATrim()
    {
        auto* pConsole = mpHost->mpConsole.data();
        auto& buffer = pConsole->buffer;
        auto& manager = pConsole->getHyperlinkVisibilityManager();
        QVERIFY(pConsole->clear(qsl("main")));
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

    // Clearing the window deletes every line, so nothing is referenced any more
    // and a tracked link's command has to go with the rest.
    void test_clearingTheWindowStillDropsATrackedLinksCommands()
    {
        auto* pConsole = mpHost->mpConsole.data();
        auto& manager = pConsole->getHyperlinkVisibilityManager();

        fill(pConsole, qsl("seed"), 5);
        const int linkId = appendLink(pConsole);
        qApp->processEvents();

        const int registeredOn = lineContaining(pConsole->buffer, mLinkText);
        QVERIFY(registeredOn >= 0);
        QVERIFY(manager.registerHyperlink(linkId, registeredOn, 0, mLinkText.length(), mLinkText, concealedRevealStyling()));
        QCOMPARE(pConsole->getLinkStore().getLinksConst(linkId), QStringList{mLinkCommand});

        QVERIFY(pConsole->clear(qsl("main")));
        qApp->processEvents();

        QVERIFY2(pConsole->getLinkStore().getLinksConst(linkId).isEmpty(), "clearing the window left the link's command behind");
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
        TChar format(pConsole);
        pConsole->buffer.addLink(false, mLinkText, commands, hints, format);
        pConsole->echo(qsl("\n"));
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

    // Fills a cleared buffer to exactly its limit, so the next line trims one
    // batch and the line indices either side of the edge are known.
    bool fillToTheBrink(TMainConsole* pConsole, const int lineNeedingRoomForALink) const
    {
        if (!pConsole->clear(qsl("main"))) {
            return false;
        }
        pConsole->buffer.setBufferSize(csmLinesLimit, csmBatchDeleteSize);
        while (static_cast<int>(pConsole->buffer.buffer.size()) < csmLinesLimit) {
            pConsole->echo(qsl("padding line %1\n").arg(pConsole->buffer.buffer.size()));
        }
        qApp->processEvents();
        return static_cast<int>(pConsole->buffer.buffer.size()) == csmLinesLimit && pConsole->buffer.lineBuffer.at(lineNeedingRoomForALink).length() >= mLinkText.length();
    }

    void tipItOver(TMainConsole* pConsole) const
    {
        pConsole->echo(qsl("the line that tips the buffer over its limit\n"));
        qApp->processEvents();
        QCOMPARE(static_cast<int>(pConsole->buffer.buffer.size()), csmLinesLimit + 1 - csmBatchDeleteSize);
    }

    void fill(TConsole* pConsole, const QString& tag, const int lines) const
    {
        for (int i = 0; i < lines; ++i) {
            pConsole->echo(qsl("%1 line %2\n").arg(tag).arg(i));
        }
    }

    int lineContaining(const TBuffer& buffer, const QString& needle) const
    {
        for (int i = 0; i < static_cast<int>(buffer.lineBuffer.size()); ++i) {
            if (buffer.lineBuffer.at(i).contains(needle)) {
                return i;
            }
        }
        return -1;
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "TrackedLinkTrimTest.moc"
MUDLET_GROUPED_TEST_MAIN(TrackedLinkTrimTest)
