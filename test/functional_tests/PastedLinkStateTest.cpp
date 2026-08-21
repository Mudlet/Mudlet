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
 * TBuffer::clearLinkState() has a fast path that skips its work when the buffer
 * has no link state to clear. paste() is why that test cannot be "did this
 * buffer's own store ever issue an id": it inserts source TChars verbatim
 * (TBuffer::paste() -> insertInLine()), unlike appendFormatted() which
 * re-registers ids through the destination's store. So a buffer can hold
 * linkIndex > 0 characters while its own TLinkStore has issued nothing, and the
 * hover path acts on such an index without checking store membership - which
 * seeds state that then has to be cleaned up like any other.
 *
 * The busted suite structurally cannot reach this: no Lua function reports link
 * state, only clearVisitedLinks() sets it.
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

    // Pins the fact the fast path cannot assume away: a pasted link id lives in a
    // buffer whose own store never issued it, so "this store is pristine" does not
    // imply "this buffer has no links".
    void test_pastedLinkIdLandsInABufferWhoseStoreIssuedNothing()
    {
        pasteLinkIntoMiniconsole();

        auto* pMini = miniconsole();
        QVERIFY2(pMini, "the miniconsole was not created");
        QVERIFY2(pMini->buffer.lineBuffer.join(QChar::LineFeed).contains(mLinkText), "the link text never reached the miniconsole, so copy()/paste() did not run");
        QVERIFY2(pastedLinkId(pMini) > 0, "paste() carried no link index across, so this test covers nothing");
        QVERIFY2(pMini->getLinkStore().pristine(), "the miniconsole's own store issued an id - paste() took appendBuffer()'s re-registering branch, not the verbatim one");
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
