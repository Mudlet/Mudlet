/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

#include <QLabel>
#include <QMenu>
#include <QWidgetAction>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TBuffer.h"
#include "TConsoleModel.h"
#include "TLinkInteraction.h"
#include "TLinkStore.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

class LinkInteractionTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = qsl("Test-LinkInteraction");
    QString mpPort;
    const QString mpLocalhost = qsl("localhost");
    Host* mpHost = nullptr;

    // Feeds one line holding an OSC 8 link to uri and returns the link's id, 0 if none arrived
    int linkTo(const QString& uri)
    {
        TBuffer& buffer = mpHost->mainConsoleView()->buffer;
        const int before = buffer.getLastLineNumber();
        QByteArray data = (qsl("\x1b]8;;") + uri + qsl("\x1b\\Text\x1b]8;;\x1b\\\r\n")).toUtf8();
        mpHost->mTelnet.loopbackTest(data);
        int linkId = 0;
        QTest::qWaitFor(
                [&]() {
                    for (int line = buffer.getLastLineNumber(); line > before - 1 && line >= 0 && !linkId; --line) {
                        linkId = buffer.getLinkIndexAt(line, 0);
                    }
                    return linkId != 0;
                },
                2000);
        return linkId;
    }

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
        TestProfile::removeProfileDirectory(mpHostname);
        mpHost = TestProfile::create(mpHostname, mpLocalhost, mpPort);
        QVERIFY(mpHost);
    }

    void cleanup()
    {
        mpHost = nullptr;
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        TestProfile::removeProfileDirectory(mpHostname);
    }

    // A note has nothing for a click to do, so the press must select text as it would anywhere else
    void test_aPressOnANoteIsAPressOnText()
    {
        TMainConsole* console = mpHost->mainConsoleView();
        const int note = linkTo(qsl("local:?config={\"tooltip\":\"It needs a silver key\"}"));
        QVERIFY(note);
        TLinkInteraction links(console->buffer, console->model(), nullptr);
        QCOMPARE(links.press(note), TLinkInteraction::Press::text);
        QCOMPARE(console->buffer.getActiveLink(), 0);
    }

    // A link whose commands were all dropped has nothing to run either
    void test_aLinkWithNoCommandsActsAsText()
    {
        TMainConsole* console = mpHost->mainConsoleView();
        TLinkInteraction links(console->buffer, console->model(), nullptr);
        const int empty = console->buffer.mLinkStore.addLinks({}, {});
        QVERIFY(empty);
        QCOMPARE(links.press(empty), TLinkInteraction::Press::text);
        QCOMPARE(console->buffer.getActiveLink(), 0);
        console->buffer.setFocusedLink(empty);
        QCOMPARE(links.activateFocused(), TLinkInteraction::Activate::ignored);
        QCOMPARE(links.menuKeyOnFocused(), TLinkInteraction::Activate::ignored);
    }

    void test_aPressOnALinkWithAMenuOpensIt()
    {
        TMainConsole* console = mpHost->mainConsoleView();
        TLinkInteraction links(console->buffer, console->model(), nullptr);
        const int local = linkTo(qsl("local:?config={\"menu\":[{\"Look\":\"send:look\"}]}"));
        QVERIFY(local);
        QCOMPARE(links.press(local), TLinkInteraction::Press::menu);
        QCOMPARE(console->buffer.getLastClickedLinkIndex(), local);
        QCOMPARE(console->buffer.getActiveLink(), 0);

        // A send: link with a menu opens it only on a right click
        const int send = linkTo(qsl("send:look?config={\"menu\":[{\"Attack\":\"send:attack\"},{\"Flee\":\"send:flee\"}]}"));
        QVERIFY(send);
        QCOMPARE(links.press(send), TLinkInteraction::Press::ran);
    }

    // The disabled look must hold, and a click must not flash the hover style
    void test_hoverLeavesADisabledOrJustClickedLinkAlone()
    {
        TMainConsole* console = mpHost->mainConsoleView();
        TLinkInteraction links(console->buffer, console->model(), nullptr);
        const int disabled = linkTo(qsl("send:look?config={\"disabled\":true}"));
        QVERIFY(disabled);
        QVERIFY(!links.hoverAt(disabled));
        QCOMPARE(console->buffer.getHoveredLink(), 0);

        const int clicked = linkTo(qsl("send:look"));
        QVERIFY(clicked);
        QCOMPARE(links.press(clicked), TLinkInteraction::Press::ran);
        QVERIFY(!links.hoverAt(clicked));
        QCOMPARE(console->buffer.getHoveredLink(), 0);
        QVERIFY(!links.leave());
        QVERIFY(links.hoverAt(clicked));
        QCOMPARE(console->buffer.getHoveredLink(), clicked);
        QVERIFY(links.leave());
        QCOMPARE(console->buffer.getHoveredLink(), 0);
    }

    void test_aRightClickOnAnUnrevealedSpoilerRevealsIt()
    {
        TMainConsole* console = mpHost->mainConsoleView();
        TLinkInteraction links(console->buffer, console->model(), nullptr);
        const int spoiler = linkTo(qsl("local:?config={\"spoiler\":true}"));
        QVERIFY(spoiler);
        QVERIFY(console->buffer.isSpoilerUnrevealed(spoiler));
        QCOMPARE(links.rightClick(spoiler), TLinkInteraction::RightClick::revealed);
        QVERIFY(!console->buffer.isSpoilerUnrevealed(spoiler));
    }

    void test_theMenuRunsAPickOfTheLinkItWasBuiltFor()
    {
        TMainConsole* console = mpHost->mainConsoleView();
        TLinkInteraction links(console->buffer, console->model(), nullptr);
        const int send = linkTo(qsl("send:look?config={\"menu\":[{\"Attack\":\"send:attack\"},{\"Flee\":\"send:flee\"}]}"));
        QVERIFY(send);
        QCOMPARE(links.rightClick(send), TLinkInteraction::RightClick::menu);
        QSignalSpy before(&links, &TLinkInteraction::aboutToRunMenuPick);
        QSignalSpy ran(&links, &TLinkInteraction::menuPickRan);
        QMenu* menu = links.buildMenu(send, nullptr, QFont(), false);
        QVERIFY(menu);
        QList<int> items;
        QAction* first = nullptr;
        for (QAction* action : menu->actions()) {
            if (const int item = action->data().toInt()) {
                items << item;
                if (item == 1) {
                    first = action;
                }
            }
        }
        QCOMPARE(items, (QList<int>{1, 2}));
        QVERIFY(first);
        first->trigger();
        QCOMPARE(before.count(), 1);
        QCOMPARE(ran.count(), 1);
        // A pick is taken once; the menu forgets its link after it
        first->trigger();
        QCOMPARE(ran.count(), 1);
        delete menu;
    }

    void test_theMenuHasItsTitleAndSeparators()
    {
        TMainConsole* console = mpHost->mainConsoleView();
        TLinkInteraction links(console->buffer, console->model(), nullptr);
        const int send = linkTo(qsl("send:attack?config={\"title\":\"Combat\",\"menu\":[{\"Strike\":\"send:strike\"},\"-\",{\"Flee\":\"send:flee\"}]}"));
        QVERIFY(send);
        std::unique_ptr<QMenu> menu(links.buildMenu(send, nullptr, QFont(), false));
        QStringList shape;
        for (QAction* action : menu->actions()) {
            if (auto* title = qobject_cast<QWidgetAction*>(action)) {
                auto* label = qobject_cast<QLabel*>(title->defaultWidget());
                shape << qsl("title:%1").arg(label ? label->text() : QString());
            } else {
                shape << (action->isSeparator() ? qsl("-") : action->text());
            }
        }
        QCOMPARE(shape, (QStringList{qsl("title:Combat"), qsl("-"), qsl("Strike"), qsl("-"), qsl("Flee")}));
    }

    // The store holds 20000 ids, so a script's flood of links can hand an open menu's id to another link
    void test_aPickRunsNothingOnceItsLinksIdIsReused()
    {
        TMainConsole* console = mpHost->mainConsoleView();
        const TLinkStore& store = console->buffer.mLinkStore;
        TLuaInterpreter* lua = mpHost->getLuaInterpreter();
        QVERIFY(lua->compileAndExecuteScript(qsl(R"lua(echoPopup("pick", {"menuPicked = 'old'", "menuPicked = 'old two'"}, {"one", "two"}))lua")));
        const int id = store.getCurrentLinkID();
        TLinkInteraction links(console->buffer, console->model(), nullptr);
        std::unique_ptr<QMenu> menu(links.buildMenu(id, nullptr, QFont(), false));
        QVERIFY(lua->compileAndExecuteScript(qsl("for i = 1, 20000 do echoLink('x', [[menuPicked = 'new']], '') end")));
        QCOMPARE(store.getCurrentLinkID(), id);
        QSignalSpy ran(&links, &TLinkInteraction::menuPickRan);
        for (QAction* action : menu->actions()) {
            if (action->data().toInt() == 1) {
                action->trigger();
            }
        }
        QCOMPARE(ran.count(), 1);
        QVERIFY2(lua->compileAndExecuteScript(qsl("assert(menuPicked == nil, menuPicked)")), "the pick ran the link that took its id");
    }

    void test_theAnnouncementNamesWhatTheLinkDoes()
    {
        TMainConsole* console = mpHost->mainConsoleView();
        TLinkInteraction links(console->buffer, console->model(), nullptr);
        const int send = linkTo(qsl("send:look"));
        QVERIFY(send);
        QCOMPARE(links.announcement(send), qsl("Send: look, link"));
        const int note = linkTo(qsl("local:?config={\"tooltip\":\"It needs a silver key\"}"));
        QVERIFY(note);
        QCOMPARE(links.announcement(note), qsl("It needs a silver key, note"));
        QCOMPARE(links.cursorFor(note), Qt::IBeamCursor);
        QCOMPARE(links.cursorFor(send), Qt::PointingHandCursor);
        QCOMPARE(links.tooltipMarkup(note), qsl("<html><body style='white-space:pre'>It needs a silver key</body></html>"));
    }
};

#include "LinkInteractionTest.moc"
MUDLET_GROUPED_TEST_MAIN(LinkInteractionTest)
