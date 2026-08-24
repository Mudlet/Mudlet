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
 * A link index only means anything to the TLinkStore that issued it. paste()
 * registers the source's links through the destination's store and writes the
 * ids that store hands back, as appendFormatted() does for the append path, so
 * a pasted link carries its own command rather than resolving against whatever
 * the destination happens to hold at that index.
 *
 * TBuffer::clearLinkState() also has a fast path that skips its work when the
 * buffer has no link state to clear, and these tests pin that a pasted id is
 * cleaned up like any other once its line goes away.
 *
 * The busted suite structurally cannot reach any of this: no Lua function
 * reports a link's command, hint or state, and only clearVisitedLinks() sets it.
 */

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TConsole.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class PastedLinkStateTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("PastedLinkState");
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mMiniName = qsl("pastedLinkMini");
    const QString mLinkText = qsl("CLICKME");

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

    // The id the pasted characters carry has to be one the destination's own
    // store issued, holding the source link's command - not an index copied
    // across that resolves against a store which never heard of it.
    void test_pastedLinkIsRegisteredInTheDestinationsOwnStore()
    {
        pasteLinkIntoMiniconsole();

        auto* pMini = miniconsole();
        QVERIFY2(pMini, "the miniconsole was not created");
        QVERIFY2(pMini->buffer.lineBuffer.join(QChar::LineFeed).contains(mLinkText), "the link text never reached the miniconsole, so copy()/paste() did not run");
        const int pastedId = pastedLinkId(pMini);
        QVERIFY2(pastedId > 0, "paste() carried no link index across, so this test covers nothing");
        QVERIFY2(!pMini->getLinkStore().pristine(), "the miniconsole's own store issued nothing, so the pasted index came across verbatim");
        QVERIFY2(pMini->getLinkStore().getLinksConst(pastedId).join(QChar::Space).contains(qsl("send('look')")), "the id the pasted characters carry does not hold the source link's command");
        QVERIFY2(pMini->getLinkStore().getHintsConst(pastedId).join(QChar::Space).contains(qsl("hint")), "the source link's hint did not come across with it");
    }

    // The issue as a player meets it: the destination already has a link of its
    // own, and ids start at 1 in every store, so a verbatim index lands on it.
    // Its own miniconsole, so the id it hands out does not depend on what an
    // earlier case left in the shared one.
    void test_pastedLinkDoesNotRunTheDestinationsOwnCommand()
    {
        const QString targetName = qsl("pastedLinkCollide");
        mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("createMiniConsole('%1', 0, 110, 300, 100)\n"
                                                                 "echoLink('%1', 'OWN', [[send('MINE')]], 'own hint')\n"
                                                                 "echo('%1', '\\none\\ntwo\\nthree\\n')\n"
                                                                 "echoLink('%2', [[send('look')]], 'hint')\n"
                                                                 "echo('\\n')")
                                                                     .arg(targetName, mLinkText));
        qApp->processEvents();

        auto* pTarget = mpHost->mpConsole->mSubConsoleMap.value(targetName);
        QVERIFY2(pTarget, "the target miniconsole was not created");
        const int ownId = pTarget->getLinkStore().getCurrentLinkID();
        QVERIFY2(ownId > 0, "the target's own echoLink() registered nothing, so there is no id to collide with");
        QVERIFY2(pTarget->getLinkStore().getLinksConst(ownId).join(QChar::Space).contains(qsl("send('MINE')")), "the target's own link is not the one we think it is");

        QVERIFY2(selectLinkRunInMainConsole(), "echoLink() put no link-bearing character in the main console");
        mpHost->mpConsole->copy();
        QVERIFY(pTarget->moveCursor(0, 0));
        pTarget->paste();
        qApp->processEvents();

        const int pastedId = pastedLinkId(pTarget);
        QVERIFY2(pastedId > 0, "paste() carried no link index across, so this test covers nothing");
        QVERIFY2(pastedId != ownId, "the pasted characters point at the target's own link id");
        const QString pastedCommands = pTarget->getLinkStore().getLinksConst(pastedId).join(QChar::Space);
        QVERIFY2(!pastedCommands.contains(qsl("send('MINE')")), "the pasted link resolves to the destination's own command");
        QVERIFY2(pastedCommands.contains(qsl("send('look')")), "the pasted link does not resolve to the command it was copied from");
    }

    // One source link can occupy more than one run of characters, with plain
    // text between them - insertText() into the middle of a link does that.
    // Remembering only the previous character's link leaves the second run
    // carrying the gap's zero, so the resumed half stops being a link at all.
    void test_aLinkResumedAfterPlainTextKeepsItsCommand()
    {
        const QString targetName = qsl("pastedLinkResumed");
        mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("createMiniConsole('%1', 0, 220, 300, 100)\n"
                                                                 "echo('%1', 'one\\ntwo\\nthree\\n')\n"
                                                                 "echoLink('RESUMEDLINK', [[send('resumed')]], 'resumed hint')\n"
                                                                 "echo('\\n')")
                                                                     .arg(targetName));
        qApp->processEvents();

        // split the link's own run with characters carrying no link index
        auto& mainBuffer = mpHost->mpConsole->buffer;
        int splitLine = -1;
        int splitColumn = -1;
        for (int y = 0, lines = static_cast<int>(mainBuffer.buffer.size()); y < lines && splitLine < 0; ++y) {
            const int at = mainBuffer.lineBuffer.at(y).indexOf(qsl("RESUMEDLINK"));
            if (at >= 0) {
                splitLine = y;
                splitColumn = at + 5;
            }
        }
        QVERIFY2(splitLine >= 0, "the link text never reached the main console");
        QPoint splitAt(splitColumn, splitLine);
        TChar plain;
        QVERIFY2(mainBuffer.insertInLine(splitAt, qsl("--"), plain), "could not split the link's run");

        const int sourceId = mainBuffer.buffer.at(splitLine).at(splitColumn - 1).linkIndex();
        QVERIFY2(sourceId > 0, "the characters before the split are not linked, so this test covers nothing");
        QCOMPARE(mainBuffer.buffer.at(splitLine).at(splitColumn).linkIndex(), 0);
        QVERIFY2(mainBuffer.buffer.at(splitLine).at(splitColumn + 2).linkIndex() == sourceId, "the link does not resume after the inserted text, so this test covers nothing");

        mpHost->mpConsole->P_begin = QPoint(splitColumn - 5, splitLine);
        mpHost->mpConsole->P_end = QPoint(splitColumn + 8, splitLine);
        mpHost->mpConsole->copy();

        auto* pTarget = mpHost->mpConsole->mSubConsoleMap.value(targetName);
        QVERIFY2(pTarget, "the target miniconsole was not created");
        QVERIFY(pTarget->moveCursor(0, 0));
        pTarget->paste();
        qApp->processEvents();

        const auto& pastedLine = pTarget->buffer.buffer.at(0);
        const int leadingId = pastedLine.at(0).linkIndex();
        QVERIFY2(leadingId > 0, "the pasted run carries no link index at all");
        // the two characters between the halves carry no link, and must not be
        // handed an id of their own - a zero index is what "not a link" means
        QCOMPARE(pastedLine.at(5).linkIndex(), 0);
        QCOMPARE(pastedLine.at(6).linkIndex(), 0);
        const int resumedId = pastedLine.at(7).linkIndex();
        QVERIFY2(resumedId > 0, "the half of the link after the inserted text lost its link index");
        QCOMPARE(resumedId, leadingId);
        QVERIFY2(pTarget->getLinkStore().getLinksConst(resumedId).join(QChar::Space).contains(qsl("send('resumed')")), "the resumed half does not resolve to the source link's command");
    }

    // The regression itself: state seeded off a pasted id has to be cleaned up
    // when the line carrying it is trimmed away, exactly as for a real link.
    void test_trimClearsLinkStateSeededByAPastedId()
    {
        auto* pMini = miniconsole();
        QVERIFY(pMini);
        auto& miniBuffer = pMini->buffer;

        // shrink first: append() is what trims, and only once the limit is exceeded
        miniBuffer.setBufferSize(100, 20);
        pasteLinkIntoMiniconsole();

        const int pastedId = pastedLinkId(pMini);
        QVERIFY(pastedId > 0);

        // what hovering such a character does, without needing a mouse event
        miniBuffer.markLinkAsVisited(pastedId);
        miniBuffer.setHoveredLink(pastedId);
        QVERIFY2(miniBuffer.isLinkVisited(pastedId), "seeding the visited state did not take");
        QCOMPARE(miniBuffer.getHoveredLink(), pastedId);

        for (int i = 0; i < 200; ++i) {
            pMini->echo(qsl("filler line %1\n").arg(i));
        }
        QVERIFY2(pastedLinkId(pMini) == 0, "the pasted line was not trimmed away, so no cleanup was due");

        QVERIFY2(!miniBuffer.isLinkVisited(pastedId), "visited state seeded by a pasted link id survived its line being trimmed away");
        QCOMPARE(miniBuffer.getHoveredLink(), 0);
    }

    // The other half of the conjunction. A link nobody ever moused over leaves
    // every state map empty and every interaction index zero, so without the
    // store's own emptiness in the test the guard would fire and
    // removeUnreferencedLinks() would never run - the store would grow for the
    // life of the profile and the links' Lua references would never be freed.
    void test_trimStillReapsAnUnreferencedLinkFromTheStore()
    {
        auto* pConsole = mpHost->mpConsole.data();
        pConsole->buffer.setBufferSize(100, 20);

        mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("echoLink('%1', [[send('look')]], 'hint')\necho('\\n')").arg(mLinkText));
        qApp->processEvents();

        const int id = pConsole->getLinkStore().getCurrentLinkID();
        QVERIFY(id > 0);
        QVERIFY2(!pConsole->getLinkStore().getLinksConst(id).isEmpty(), "echoLink() did not register the link in the store");

        for (int i = 0; i < 200; ++i) {
            pConsole->echo(qsl("filler line %1\n").arg(i));
        }

        QVERIFY2(pConsole->getLinkStore().getLinksConst(id).isEmpty(), "an unreferenced link survived its line being trimmed away, so the store grows for the life of the profile");
    }

private:
    TConsole* miniconsole() const { return mpHost->mpConsole->mSubConsoleMap.value(mMiniName); }

    // Highest link index still present in a console's buffer, 0 for none
    int pastedLinkId(TConsole* pConsole) const
    {
        int found = 0;
        for (const auto& line : pConsole->buffer.buffer) {
            for (const TChar& character : line) {
                found = qMax(found, character.linkIndex());
            }
        }
        return found;
    }

    // Sets the console's selection to the run of link-bearing characters, so
    // copy() takes that span without depending on selectString() finding text on
    // whichever line it considers current
    bool selectLinkRunInMainConsole() const
    {
        const auto& mainBuffer = mpHost->mpConsole->buffer;
        for (int y = 0, lines = static_cast<int>(mainBuffer.buffer.size()); y < lines; ++y) {
            const auto& line = mainBuffer.buffer.at(y);
            int from = -1;
            int to = -1;
            for (int x = 0, columns = static_cast<int>(line.size()); x < columns; ++x) {
                if (line.at(x).linkIndex() > 0) {
                    from = (from < 0) ? x : from;
                    to = x + 1;
                }
            }
            if (from >= 0) {
                mpHost->mpConsole->P_begin = QPoint(from, y);
                mpHost->mpConsole->P_end = QPoint(to, y);
                return true;
            }
        }
        return false;
    }

    // TConsole::paste() only takes TBuffer::paste()'s verbatim path when the
    // cursor is above the last line - an empty target goes through appendBuffer()
    // instead, which re-registers the id and so is not the path under test.
    void pasteLinkIntoMiniconsole() const
    {
        mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("if not exists('%1', 'window') then createMiniConsole('%1', 0, 0, 300, 100) end\n"
                                                                 "echo('%1', 'one\\ntwo\\nthree\\n')\n"
                                                                 "echoLink('%2', [[send('look')]], 'hint')\n"
                                                                 "echo('\\n')")
                                                                     .arg(mMiniName, mLinkText));
        qApp->processEvents();

        QVERIFY2(selectLinkRunInMainConsole(), "echoLink() put no link-bearing character in the main console");
        mpHost->mpConsole->copy();

        auto* pMini = miniconsole();
        QVERIFY(pMini);
        QVERIFY(pMini->moveCursor(0, 0));
        pMini->paste();
        qApp->processEvents();
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "PastedLinkStateTest.moc"
MUDLET_GROUPED_TEST_MAIN(PastedLinkStateTest)
