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

/*
 * There is one speech recognizer for the whole application - one microphone,
 * one decoder - so at most one profile can be listening, and everything the
 * session produces belongs to the profile that started it rather than to
 * whichever one happens to be in front when a phrase lands. Those are the same
 * profile in the ordinary case, which is why routing by the second one went
 * unnoticed: it is wrong only when they differ, and they can only differ with
 * two profiles open.
 *
 * That is what puts this in a functional test rather than a spec, per the
 * testing note in CLAUDE.md: a spec runs inside one profile and cannot open a
 * second, so none of the cases here are reachable from one.
 *
 * No engine is installed in a test run, so none of this drives a real
 * recognizer. It does not need to - the ownership and the routing are core's,
 * and raiseSpeechEvent() is the seam every backend's results arrive through.
 *
 * Run with: ctest -R SpeechAcrossProfilesTest -V
 */

#include <QAction>
#include <QFileInfo>
#include <QMenu>
#include <QToolBar>
#include <QToolButton>
#include <QWidgetAction>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "ProfileTestHelper.h"
#include "TLuaInterpreter.h"
#include "TDetachedWindow.h"
#include "TTabBar.h"
#include "TelnetServerStub.h"
#include "mudlet.h"

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lauxlib.h>
#include <lua5.1/lua.h>
#include <lua5.1/lualib.h>
#else
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#endif
}

#include "GroupedTest.h"

using namespace std::chrono_literals;

class SpeechAcrossProfilesTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpFirstHost = nullptr;
    Host* mpSecondHost = nullptr;
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mFirstHostname = qsl("SpeechAcrossProfiles-First");
    const QString mSecondHostname = qsl("SpeechAcrossProfiles-Second");

    Host* hostFor(const QString& profileName) const { return mudlet::self()->getHostManager().getHost(profileName); }

    // The connection dialog can only be driven once: with the main window up it
    // never takes activation again, so the second profile is opened the way a
    // player opens one from inside a running Mudlet instead.
    bool provisionProfileOnDisk(const QString& profileName) const
    {
        return QDir().mkpath(mudlet::getMudletPath(enums::profileHomePath, profileName)) && mudlet::self()->writeProfileData(profileName, qsl("url"), mLocalhost).first
               && mudlet::self()->writeProfileData(profileName, qsl("port"), mPort).first;
    }

    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    // Returns the Lua error, or a null QString when the chunk ran
    QString runLua(Host* pHost, const QString& code) const
    {
        lua_State* L = pHost->getLuaInterpreter()->getLuaGlobalState();
        if (luaL_dostring(L, code.toUtf8().constData()) == 0) {
            return QString();
        }
        const char* message = lua_tostring(L, -1);
        const QString error = message ? QString::fromUtf8(message) : qsl("(a Lua error that is not a string)");
        lua_pop(L, 1);
        return error;
    }

    QString luaGlobalString(Host* pHost, const QString& globalName) const
    {
        lua_State* L = pHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, globalName.toUtf8().constData());
        const char* value = lua_tostring(L, -1);
        const QString result = value ? QString::fromUtf8(value) : QString();
        lua_pop(L, 1);
        return result;
    }

    // Records the argument of one sysSTT* event into a global of this profile's
    // own, so that "which profile heard it" is a question each Lua state
    // answers for itself rather than one the test has to infer.
    void listenFor(Host* pHost, const QString& eventName, const QString& globalName) const
    {
        const QString code = qsl("%1 = nil registerAnonymousEventHandler(\"%2\", function(_, value) %1 = value end)").arg(globalName, eventName);
        QVERIFY2(runLua(pHost, code).isNull(), qPrintable(qsl("could not arm a handler for %1").arg(eventName)));
    }

    bool luaGlobalBoolean(Host* pHost, const QString& globalName) const
    {
        lua_State* L = pHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, globalName.toUtf8().constData());
        const bool value = lua_toboolean(L, -1);
        lua_pop(L, 1);
        return value;
    }

    int luaGlobalNumber(Host* pHost, const QString& globalName) const
    {
        lua_State* L = pHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, globalName.toUtf8().constData());
        const int value = static_cast<int>(lua_tonumber(L, -1));
        lua_pop(L, 1);
        return value;
    }

    // The id a Lua call returned, or -1 when the command was refused
    int addCommand(Host* pHost, const QString& fields) const
    {
        const QString error = runLua(pHost, qsl("_addonId = addCommand{%1} or -1").arg(fields));
        if (!error.isNull()) {
            qWarning() << "addCommand:" << error;
            return -1;
        }
        return luaGlobalNumber(pHost, qsl("_addonId"));
    }

    // The button a command placed, searched for in one window only - which is
    // the whole question here, since the bug being pinned is a button appearing
    // in a window that is not its profile's.
    QToolButton* buttonIn(QWidget* pContainer, const QString& name) const { return pContainer->findChild<QToolButton*>(qsl("addon_%1").arg(name), Qt::FindChildrenRecursively); }

    // The toolbar entry a command placed. Asked for rather than the button's own
    // isVisible(), which in an offscreen run is false for every widget in a
    // window that was never shown, and rather than isHidden(), which a toolbar
    // that is itself hidden sets on the buttons inside it. The action's
    // visibility is the command's own answer either way.
    QAction* toolbarEntryIn(QWidget* pContainer, const QString& name) const
    {
        QToolButton* pButton = buttonIn(pContainer, name);
        if (!pButton) {
            return nullptr;
        }
        for (QToolBar* pToolBar : pContainer->findChildren<QToolBar*>()) {
            for (QAction* pAction : pToolBar->actions()) {
                auto* pWidgetAction = qobject_cast<QWidgetAction*>(pAction);
                if (pWidgetAction && pWidgetAction->defaultWidget() == pButton) {
                    return pAction;
                }
            }
        }
        return nullptr;
    }

    QAction* menuItemIn(QWidget* pContainer, const QString& name) const
    {
        for (QAction* pAction : pContainer->findChildren<QAction*>()) {
            if (pAction->text() == name) {
                return pAction;
            }
        }
        return nullptr;
    }

    void deleteProfileDirectory(const QString& profileName) const
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->serverPort() != 0, "the telnet stub did not start listening");
        mPort = QString::number(mpServer->serverPort());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::getQSettings()->setValue(qsl("uiTourShown"), true);
        mudlet::getQSettings()->sync();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        deleteProfileDirectory(mFirstHostname);
        deleteProfileDirectory(mSecondHostname);

        mpFirstHost = TestProfile::create(mFirstHostname, mLocalhost, mPort);
        QVERIFY2(mpFirstHost, "the first profile did not load");

        QVERIFY(provisionProfileOnDisk(mSecondHostname));
        QVERIFY2(runLua(mpFirstHost, qsl("loadProfile('%1', true)").arg(mSecondHostname)).isNull(), "the second profile could not be loaded");
        QTest::qWait(500ms);
        mpSecondHost = hostFor(mSecondHostname);
        QVERIFY2(mpSecondHost, "the second profile did not open");
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory(mFirstHostname);
            deleteProfileDirectory(mSecondHostname);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // The main toolbar is hidden by default, and a toolbar-only command is
    // refused while it is - which would leave every placement case below
    // asserting against a command that was never created.
    void init() { mudlet::self()->setToolBarVisibility(enums::visibleAlways); }

    // No test may leave the microphone claimed: the owner decides where the
    // next test's events land, so a leaked claim fails the one after it
    void cleanup() { mudlet::self()->releaseMicrophone(); }

    // The case the whole ownership model exists for. Listening starts in one
    // profile, the player tabs to the other, and the phrase they finish saying
    // belongs to the game they said it to - not to the one now in front.
    void test_aResultGoesToTheProfileHoldingTheMicrophone()
    {
        listenFor(mpFirstHost, qsl("sysSTTResult"), qsl("_heardFirst"));
        listenFor(mpSecondHost, qsl("sysSTTResult"), qsl("_heardSecond"));

        mudlet::self()->claimMicrophoneFor(mpFirstHost);
        mudlet::self()->activateProfile(mpSecondHost);
        QCOMPARE(mudlet::self()->getActiveHost(), mpSecondHost);

        mudlet::self()->raiseSpeechEvent(qsl("sysSTTResult"), qsl("kill hound"));

        QCOMPARE(luaGlobalString(mpFirstHost, qsl("_heardFirst")), qsl("kill hound"));
        QVERIFY2(luaGlobalString(mpSecondHost, qsl("_heardSecond")).isEmpty(), "the phrase was delivered to the profile in front rather than to the one that was listening");
    }

    // The other half of the same rule: with no session running there is no
    // owner to prefer, and a refusal or a capability change still has to reach
    // somebody. The profile in front is the right answer then, and only then.
    void test_withNoOwnerAnEventGoesToTheProfileInFront()
    {
        listenFor(mpFirstHost, qsl("sysSTTError"), qsl("_faultFirst"));
        listenFor(mpSecondHost, qsl("sysSTTError"), qsl("_faultSecond"));

        mudlet::self()->releaseMicrophone();
        mudlet::self()->activateProfile(mpSecondHost);

        mudlet::self()->raiseSpeechEvent(qsl("sysSTTError"), qsl("no model"));

        QCOMPARE(luaGlobalString(mpSecondHost, qsl("_faultSecond")), qsl("no model"));
        QVERIFY2(luaGlobalString(mpFirstHost, qsl("_faultFirst")).isEmpty(), "an event with no session running went somewhere other than the profile in front");
    }

    // A second profile asking to listen is a handover, and the profile that
    // loses the microphone is told by name. Nothing else on its screen would
    // say why it stopped: its own button simply goes quiet.
    void test_takingTheMicrophoneTellsTheProfileThatLostIt()
    {
        listenFor(mpFirstHost, qsl("sysSTTHandover"), qsl("_handoverFirst"));
        listenFor(mpSecondHost, qsl("sysSTTHandover"), qsl("_handoverSecond"));

        mudlet::self()->claimMicrophoneFor(mpFirstHost);
        mudlet::self()->claimMicrophoneFor(mpSecondHost);

        QCOMPARE(mudlet::self()->microphoneOwner(), mpSecondHost);
        QCOMPARE(luaGlobalString(mpFirstHost, qsl("_handoverFirst")), mSecondHostname);
        QVERIFY2(luaGlobalString(mpSecondHost, qsl("_handoverSecond")).isEmpty(), "the profile that took the microphone was told it had lost one");
    }

    // Re-asking for a microphone a profile already holds is not a handover, and
    // announcing one would have every repeated stt.start() tell the profile it
    // had lost the session it is still running.
    void test_askingTwiceFromOneProfileAnnouncesNothing()
    {
        listenFor(mpFirstHost, qsl("sysSTTHandover"), qsl("_handoverAgain"));

        mudlet::self()->claimMicrophoneFor(mpFirstHost);
        mudlet::self()->claimMicrophoneFor(mpFirstHost);

        QCOMPARE(mudlet::self()->microphoneOwner(), mpFirstHost);
        QVERIFY2(luaGlobalString(mpFirstHost, qsl("_handoverAgain")).isEmpty(), "a profile that kept the microphone was told it had lost it");
    }

    // Releasing is what the end of a session does, and it has to put routing
    // back to the profile in front rather than leaving the last owner latched.
    void test_releasingHandsRoutingBackToTheProfileInFront()
    {
        listenFor(mpFirstHost, qsl("sysSTTStateChanged"), qsl("_stateFirst"));
        listenFor(mpSecondHost, qsl("sysSTTStateChanged"), qsl("_stateSecond"));

        mudlet::self()->claimMicrophoneFor(mpFirstHost);
        mudlet::self()->activateProfile(mpSecondHost);
        mudlet::self()->releaseMicrophone();

        mudlet::self()->raiseSpeechEvent(qsl("sysSTTStateChanged"), qsl("ready"));

        QCOMPARE(luaGlobalString(mpSecondHost, qsl("_stateSecond")), qsl("ready"));
        QVERIFY2(luaGlobalString(mpFirstHost, qsl("_stateFirst")).isEmpty(), "a released microphone left its old owner still receiving the session's events");
    }

    // The duplicate the player reported: every open profile placed its command
    // in the main window's toolbar, so two games meant two identical buttons
    // with nothing but a tooltip to tell them apart.
    void test_onlyTheShownProfilesCommandIsOnTheToolbar()
    {
        const int firstId = addCommand(mpFirstHost, qsl("name = \"SpeechFirst\", surfaces = \"toolbar\""));
        const int secondId = addCommand(mpSecondHost, qsl("name = \"SpeechSecond\", surfaces = \"toolbar\""));
        QVERIFY(firstId > 0 && secondId > 0);

        mudlet::self()->activateProfile(mpFirstHost);
        QAction* pFirstEntry = toolbarEntryIn(mudlet::self(), qsl("SpeechFirst"));
        QAction* pSecondEntry = toolbarEntryIn(mudlet::self(), qsl("SpeechSecond"));
        QVERIFY2(pFirstEntry && pSecondEntry, "both commands should exist in the main window, whatever their visibility");
        QVERIFY2(pFirstEntry->isVisible(), "the shown profile's button is not on the toolbar");
        QVERIFY2(!pSecondEntry->isVisible(), "a profile that is not being shown still has a button on the toolbar");

        mudlet::self()->activateProfile(mpSecondHost);
        QVERIFY2(!pFirstEntry->isVisible(), "switching profiles left the old profile's button behind");
        QVERIFY2(pSecondEntry->isVisible(), "switching profiles did not bring the new profile's button out");

        runLua(mpFirstHost, qsl("removeCommand(%1)").arg(firstId));
        runLua(mpSecondHost, qsl("removeCommand(%1)").arg(secondId));
    }

    // The menu half goes with it, and takes its shortcut: a key that raises
    // another game's event while you look at this one is the same mistake as
    // a button that does.
    void test_aHiddenProfilesMenuItemIsHiddenToo()
    {
        const int secondId = addCommand(mpSecondHost, qsl("name = \"MenuSecond\", menuPath = \"Speech\", surfaces = \"menu\""));
        QVERIFY(secondId > 0);

        mudlet::self()->activateProfile(mpSecondHost);
        QAction* pItem = menuItemIn(mudlet::self(), qsl("MenuSecond"));
        QVERIFY2(pItem, "the menu item was never placed");
        QVERIFY2(pItem->isVisible(), "the shown profile's menu item is not on the menu");

        mudlet::self()->activateProfile(mpFirstHost);
        QVERIFY2(!pItem->isVisible(), "another profile's menu item is still on the menu");

        runLua(mpSecondHost, qsl("removeCommand(%1)").arg(secondId));
    }

    // A detached profile's button used to stay in the main window, which is the
    // one window it certainly does not belong in.
    void test_detachingTakesTheCommandToItsNewWindow()
    {
        const int secondId = addCommand(mpSecondHost, qsl("name = \"SpeechMoved\", surfaces = \"toolbar\""));
        QVERIFY(secondId > 0);
        QVERIFY2(buttonIn(mudlet::self(), qsl("SpeechMoved")), "the command was not placed in the main window to begin with");

        TDetachedWindow* pWindow = detachSecondProfile();
        QVERIFY(pWindow);

        QVERIFY2(!buttonIn(mudlet::self(), qsl("SpeechMoved")), "the command stayed in the main window after its profile was detached");
        QVERIFY2(buttonIn(pWindow, qsl("SpeechMoved")), "the command did not arrive in the detached window");
        QAction* pMovedEntry = toolbarEntryIn(pWindow, qsl("SpeechMoved"));
        QVERIFY2(pMovedEntry && pMovedEntry->isVisible(), "the command arrived in the detached window but is not shown, though that window shows its profile");

        mudlet::self()->slot_tabReattachRequested(mSecondHostname);
        QTest::qWait(200ms);
        QVERIFY2(buttonIn(mudlet::self(), qsl("SpeechMoved")), "reattaching did not bring the command back to the main window");

        runLua(mpSecondHost, qsl("removeCommand(%1)").arg(secondId));
    }

    // The widgets are rebuilt on a move, so everything the package set has to
    // be re-applied to them - it lives on the command, not on the button.
    void test_aMovedCommandKeepsWhatThePackageSet()
    {
        const int secondId = addCommand(mpSecondHost, qsl("name = \"SpeechState\", surfaces = \"toolbar\""));
        QVERIFY(secondId > 0);
        runLua(mpSecondHost, qsl("setCommandChecked(%1, true)").arg(secondId));
        runLua(mpSecondHost, qsl("setCommandTooltip(%1, \"listening in StickMUD\")").arg(secondId));
        runLua(mpSecondHost, qsl("disableCommand(%1)").arg(secondId));

        TDetachedWindow* pWindow = detachSecondProfile();
        QVERIFY(pWindow);

        QToolButton* pMoved = buttonIn(pWindow, qsl("SpeechState"));
        QVERIFY2(pMoved, "the command did not arrive in the detached window");
        QVERIFY2(pMoved->isChecked(), "a checked command came out of the move unchecked");
        QVERIFY2(pMoved->toolTip().contains(qsl("listening in StickMUD")), qPrintable(qsl("the tooltip did not survive the move: %1").arg(pMoved->toolTip())));
        QVERIFY2(!pMoved->isEnabled(), "a disabled command came out of the move enabled");

        mudlet::self()->slot_tabReattachRequested(mSecondHostname);
        QTest::qWait(200ms);
        runLua(mpSecondHost, qsl("removeCommand(%1)").arg(secondId));
    }

    // What the player asked for: a live microphone they can always see and stop,
    // without the two idle buttons that made them ask. A pinned command is shown
    // in the window they are in even while it shows another profile - and only
    // one command is ever pinned, the one that is actually doing something.
    void test_aPinnedCommandIsShownWhileAnotherProfileIsInFront()
    {
        const int secondId = addCommand(mpSecondHost, qsl("name = \"SpeechPinned\", surfaces = \"toolbar\""));
        QVERIFY(secondId > 0);

        mudlet::self()->activateProfile(mpFirstHost);
        QAction* pEntry = toolbarEntryIn(mudlet::self(), qsl("SpeechPinned"));
        QVERIFY2(pEntry, "the command was not placed at all");
        QVERIFY2(!pEntry->isVisible(), "an unpinned command of another profile is on the toolbar");

        QVERIFY(runLua(mpSecondHost, qsl("setCommandPinned(%1, true)").arg(secondId)).isNull());
        QVERIFY2(pEntry->isVisible(), "a pinned command is not shown while another profile is in front");

        QVERIFY(runLua(mpSecondHost, qsl("setCommandPinned(%1, false)").arg(secondId)).isNull());
        QVERIFY2(!pEntry->isVisible(), "unpinning did not put the command back with its own profile");

        runLua(mpSecondHost, qsl("removeCommand(%1)").arg(secondId));
    }

    // Pinning is the profile's own business, like every other command operation:
    // an id belonging to somebody else answers as an unknown one does.
    void test_pinningIsRefusedForAnotherProfilesCommand()
    {
        const int secondId = addCommand(mpSecondHost, qsl("name = \"SpeechNotYours\", surfaces = \"toolbar\""));
        QVERIFY(secondId > 0);

        QVERIFY(runLua(mpFirstHost, qsl("_pinnedOther = setCommandPinned(%1, true)").arg(secondId)).isNull());
        QVERIFY2(!luaGlobalBoolean(mpFirstHost, qsl("_pinnedOther")), "a profile pinned another profile's command");

        QVERIFY(runLua(mpFirstHost, qsl("_pinnedUnknown = setCommandPinned(999999, true)")).isNull());
        QVERIFY2(!luaGlobalBoolean(mpFirstHost, qsl("_pinnedUnknown")), "an unknown id was accepted");

        runLua(mpSecondHost, qsl("removeCommand(%1)").arg(secondId));
    }

    // The one signal that survives the window being minimised or behind
    // something else, which is exactly the case "stt focus keep" creates.
    void test_theWindowTitleSaysWhenItsProfileHasTheMicrophone()
    {
        mudlet::self()->activateProfile(mpFirstHost);
        const QString quiet = mudlet::self()->windowTitle();
        QVERIFY2(!quiet.contains(qsl("listening")), "the title claimed a microphone before one was open");

        mudlet::self()->claimMicrophoneFor(mpFirstHost);
        QVERIFY2(mudlet::self()->windowTitle().contains(qsl("listening")), qPrintable(qsl("the title does not say the microphone is open: %1").arg(mudlet::self()->windowTitle())));

        mudlet::self()->releaseMicrophone();
        QCOMPARE(mudlet::self()->windowTitle(), quiet);
    }

    // A profile in another window holding the microphone is not this window's
    // news, or every window would claim the one microphone at once.
    void test_onlyTheOwningWindowsTitleIsMarked()
    {
        mudlet::self()->activateProfile(mpFirstHost);
        TDetachedWindow* pWindow = detachSecondProfile();
        QVERIFY(pWindow);

        mudlet::self()->claimMicrophoneFor(mpSecondHost);
        QVERIFY2(pWindow->windowTitle().contains(qsl("listening")), qPrintable(qsl("the detached window holding the microphone does not say so: %1").arg(pWindow->windowTitle())));
        QVERIFY2(!mudlet::self()->windowTitle().contains(qsl("listening")), "the main window claimed a microphone belonging to a profile in another window");

        mudlet::self()->releaseMicrophone();
        mudlet::self()->slot_tabReattachRequested(mSecondHostname);
        QTest::qWait(200ms);
    }

    // Dragging a profile out of the main window while it is listening moves its
    // control to the new window and the marker with it. The claim itself must
    // not move: the session is still running and still belongs to that profile,
    // whichever window is now drawing it.
    void test_detachingAProfileThatHoldsTheMicrophoneCarriesBothWithIt()
    {
        // Deliberately not pinned: a pinned command follows the player rather
        // than its profile, and which window has focus after a detach is the
        // window manager's business - not something to assert offscreen. Where
        // a pinned one goes is covered by the pinning case above.
        const int secondId = addCommand(mpSecondHost, qsl("name = \"SpeechLive\", surfaces = \"toolbar\""));
        QVERIFY(secondId > 0);
        mudlet::self()->claimMicrophoneFor(mpSecondHost);

        TDetachedWindow* pWindow = detachSecondProfile();
        QVERIFY(pWindow);

        QCOMPARE(mudlet::self()->microphoneOwner(), mpSecondHost);
        QVERIFY2(pWindow->windowTitle().contains(qsl("listening")), "the detached window does not say it took the open microphone with it");
        QVERIFY2(buttonIn(pWindow, qsl("SpeechLive")), "the live command did not follow its profile out of the main window");

        QVERIFY2(!buttonIn(mudlet::self(), qsl("SpeechLive")), "the command stayed in the main window while its listening profile left");

        mudlet::self()->releaseMicrophone();
        QVERIFY2(!pWindow->windowTitle().contains(qsl("listening")), "the marker outlived the session it was describing");

        mudlet::self()->slot_tabReattachRequested(mSecondHostname);
        QTest::qWait(200ms);
        runLua(mpSecondHost, qsl("removeCommand(%1)").arg(secondId));
    }

private:
    TDetachedWindow* detachSecondProfile()
    {
        // Whichever profile sits at tab 1 detaches, and slot_tabDetachRequested
        // refuses index 0 - so this is the second profile by construction
        mudlet::self()->slot_tabDetachRequested(1, QPoint(200, 200));
        QTest::qWait(200ms);
        return mudlet::self()->getDetachedWindows().value(mSecondHostname);
    }
};

#include "SpeechAcrossProfilesTest.moc"
MUDLET_GROUPED_TEST_MAIN(SpeechAcrossProfilesTest)
