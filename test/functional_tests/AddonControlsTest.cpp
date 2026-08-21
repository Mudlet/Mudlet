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
 * The addon command API of docs/addon-ui-api.md is called from Lua, but what
 * is held here is not reachable from a spec: a click has to come from a real
 * QToolButton or QAction, the submenu pruning is Qt object lifetime across an
 * event loop turn, ownership means nothing until a second profile is open,
 * and closing or resetting a profile is not something a script can watch from
 * inside itself.
 *
 * The cheaper surface - icons, tooltips, checked state, pulse, unknown ids -
 * is covered by AddonCommand_spec.lua instead, per the testing note in
 * CLAUDE.md: a spec costs ~30KB and no rebuild, a functional test ~290MB and
 * a link step.
 *
 * Run with: ctest -R AddonControlsTest -V
 */


#include <QAction>
#include <QMenu>
#include <QSignalSpy>
#include <QToolButton>
#include <QtTest/QtTest>
#include <chrono>

#include "PortableModeTestHelper.h"
#include "TMainConsole.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
#include "dlgConnectionProfiles.h"
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

class AddonControlsTest : public QObject
{
    Q_OBJECT

private:
    static constexpr int csmConnectBudgetMs = 2000;

    TelnetServerStub* mpServer = nullptr;
    Host* mpFirstHost = nullptr;
    Host* mpSecondHost = nullptr;
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mFirstProfile = qsl("AddonControls-First");
    const QString mSecondProfile = qsl("AddonControls-Second");
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdgConfigHome;

    Host* hostFor(const QString& profileName) const { return mudlet::self()->getHostManager().getHost(profileName); }

    bool provisionProfileOnDisk(const QString& profileName) const
    {
        return QDir().mkpath(mudlet::getMudletPath(enums::profileHomePath, profileName)) && mudlet::self()->writeProfileData(profileName, qsl("url"), mLocalhost).first
               && mudlet::self()->writeProfileData(profileName, qsl("port"), mPort).first;
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

    int luaGlobalNumber(Host* pHost, const QString& globalName) const
    {
        lua_State* L = pHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, globalName.toUtf8().constData());
        const int value = static_cast<int>(lua_tonumber(L, -1));
        lua_pop(L, 1);
        return value;
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

    bool luaGlobalBoolean(Host* pHost, const QString& globalName) const
    {
        lua_State* L = pHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, globalName.toUtf8().constData());
        const bool value = lua_toboolean(L, -1);
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

    // The reason a refused command gave, which is the half a package can act on
    QString refusalReason(Host* pHost, const QString& fields) const
    {
        const QString error = runLua(pHost, qsl("_addonId, _addonWhy = addCommand{%1}").arg(fields));
        if (!error.isNull()) {
            return error;
        }
        return luaGlobalString(pHost, qsl("_addonWhy"));
    }

    // A binding that refused and a chunk that never ran both used to arrive
    // here as "false", so a stubbed implementation passed every negative
    // assertion. The Lua error is now kept apart from the binding's answer.
    mutable QString mLastLuaError;

    bool callRan(Host* pHost, const QString& call, bool& answer) const
    {
        mLastLuaError = runLua(pHost, qsl("_addonResult = %1").arg(call));
        if (!mLastLuaError.isNull()) {
            return false;
        }
        answer = luaGlobalBoolean(pHost, qsl("_addonResult"));
        return true;
    }

    // Whether a boolean-returning binding said yes, failing the test if the
    // call itself could not run
    bool callReturnedTrue(Host* pHost, const QString& call) const
    {
        bool answer = false;
        if (!callRan(pHost, call, answer)) {
            qWarning() << "the call did not run:" << call << mLastLuaError;
            return false;
        }
        return answer;
    }

    // Whether a binding refused - which is not the same as failing to run
    bool callRefused(Host* pHost, const QString& call) const
    {
        bool answer = true;
        if (!callRan(pHost, call, answer)) {
            return false;
        }
        return !answer;
    }

    QToolButton* toolbarButtonNamed(const QString& name) const { return mudlet::self()->findChild<QToolButton*>(qsl("addon_%1").arg(name)); }

    // Menu items carry no object name, so they are found the way a user finds
    // them - by the text on them
    QAction* menuActionNamed(const QString& name) const
    {
        const QList<QAction*> actions = mudlet::self()->findChildren<QAction*>();
        for (QAction* action : actions) {
            if (!action->menu() && action->text() == name) {
                return action;
            }
        }
        return nullptr;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - cannot redirect the config dir for this test");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdgConfigHome = qgetenv("XDG_CONFIG_HOME");
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

        const QString profileName = mFirstProfile;
        const QString address = mLocalhost;
        const QString port = mPort;
        QTimer::singleShot(0ms, qApp, [profileName, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100ms);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), profileName);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), address);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        QVERIFY2(spy.wait(csmConnectBudgetMs), "the first profile took too long to load");
        mpFirstHost = mudlet::self()->getActiveHost();
        QVERIFY2(mpFirstHost, "no active host after creating the first profile");
        QVERIFY2(mpFirstHost->mFORCE_SAVE_ON_EXIT, "profiles must save without asking, or a close puts up a modal question");

        QVERIFY(provisionProfileOnDisk(mSecondProfile));
        QVERIFY2(runLua(mpFirstHost, qsl("loadProfile('%1', true)").arg(mSecondProfile)).isNull(), "the second profile could not be loaded");
        QTest::qWait(500ms);
        mpSecondHost = hostFor(mSecondProfile);
        QVERIFY2(mpSecondHost, "the second profile did not open");
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpFirstHost = nullptr;
        mpSecondHost = nullptr;
        delete mudlet::self();
        if (mSavedXdgConfigHome.isEmpty()) {
            qunsetenv("XDG_CONFIG_HOME");
        } else {
            qputenv("XDG_CONFIG_HOME", mSavedXdgConfigHome);
        }
    }

    // docs/addon-ui-api.md gives the click event the id as addCommand returned
    // it, which is a number - the same as every other Mudlet event carrying one
    void test_aClickHandsTheHandlerTheIdAsANumber()
    {
        QVERIFY2(runLua(mpFirstHost,
                        qsl("_addonSeenType, _addonSeenId, _addonSeenCount = nil, nil, 0\n"
                            "function _addonClickHandler(event, id)\n"
                            "  _addonSeenType = type(id)\n"
                            "  _addonSeenId = tostring(id)\n"
                            "  _addonSeenCount = _addonSeenCount + 1\n"
                            "end\n"
                            "registerAnonymousEventHandler('sysCommandClicked', '_addonClickHandler')"))
                         .isNull(),
                 "the click handler could not be registered");

        const int commandId = addCommand(mpFirstHost, qsl("name = 'Clicked', menuPath = 'ClickTest'"));
        QVERIFY2(commandId > 0, "the command was not placed");

        QToolButton* pButton = toolbarButtonNamed(qsl("Clicked"));
        QVERIFY2(pButton, "the command is not on the toolbar");
        pButton->click();
        QTest::qWait(100ms);
        QCOMPARE(luaGlobalString(mpFirstHost, qsl("_addonSeenType")), qsl("number"));
        QCOMPARE(luaGlobalString(mpFirstHost, qsl("_addonSeenId")), QString::number(commandId));

        // the same command from its other surface: one id, one event, so a
        // package never has to map two ids back onto one handler
        QAction* pAction = menuActionNamed(qsl("Clicked"));
        QVERIFY2(pAction, "the command is not in the menu");
        pAction->trigger();
        QTest::qWait(100ms);
        QCOMPARE(luaGlobalString(mpFirstHost, qsl("_addonSeenId")), QString::number(commandId));
        QCOMPARE(luaGlobalNumber(mpFirstHost, qsl("_addonSeenCount")), 2);

        QVERIFY(callReturnedTrue(mpFirstHost, qsl("removeCommand(%1)").arg(commandId)));
        QTest::qWait(100ms);
        QVERIFY2(!toolbarButtonNamed(qsl("Clicked")), "removing the command left its button behind");
        QVERIFY2(!menuActionNamed(qsl("Clicked")), "removing the command left its menu entry behind");
    }

    // A sysCommandClicked handler that removes the command it was called for
    // and puts another at the same path runs entirely inside one turn of the
    // event loop, so the add sees a submenu the remove has already condemned
    void test_aPathReusedInTheSameTurnKeepsItsNewItem()
    {
        QVERIFY2(runLua(mpFirstHost,
                        qsl("_addonFirstId = addCommand{name = 'FirstItem', menuPath = 'Reuse/Nested'}\n"
                            "_addonRemoved = removeCommand(_addonFirstId)\n"
                            "_addonSecondId = addCommand{name = 'SecondItem', menuPath = 'Reuse/Nested'}"))
                         .isNull(),
                 "the remove-then-add chunk did not run");
        QVERIFY(luaGlobalBoolean(mpFirstHost, qsl("_addonRemoved")));
        const int secondId = luaGlobalNumber(mpFirstHost, qsl("_addonSecondId"));
        QVERIFY2(secondId > 0, "the second command was not placed");

        // the deletions the removal deferred happen here
        QTest::qWait(200ms);
        QCoreApplication::processEvents();

        QVERIFY2(menuActionNamed(qsl("SecondItem")), "the command added at the reused path is gone once the deferred deletions have run");
        QVERIFY2(callReturnedTrue(mpFirstHost, qsl("disableCommand(%1)").arg(secondId)), "the reused path's command no longer answers to its own id");

        QVERIFY(callReturnedTrue(mpFirstHost, qsl("removeCommand(%1)").arg(secondId)));
        QTest::qWait(100ms);
    }

    // Ids come from one sequence, so a second profile can name the first
    // profile's commands whether or not it created them
    void test_aProfileCannotTouchAnotherProfilesCommands()
    {
        const int firstId = addCommand(mpFirstHost, qsl("name = 'OwnedByFirst', menuPath = 'Owned'"));
        QVERIFY2(firstId > 0, "the first profile's command was not placed");

        // A positive control, so these assertions cannot pass merely because
        // the second profile's interpreter is unusable - and it covers the
        // other direction of the same rule in the same breath
        const int secondId = addCommand(mpSecondHost, qsl("name = 'OwnedBySecond', menuPath = 'Owned'"));
        QVERIFY2(secondId > 0, "the second profile could not place a command of its own");
        QVERIFY2(secondId != firstId, "the two profiles were handed the same id");
        QVERIFY2(callReturnedTrue(mpSecondHost, qsl("disableCommand(%1)").arg(secondId)), "the second profile could not address its own command");
        QVERIFY2(callReturnedTrue(mpSecondHost, qsl("setCommandTooltip(%1, 'mine')").arg(secondId)), "the second profile could not retitle its own command");

        QVERIFY2(callRefused(mpSecondHost, qsl("disableCommand(%1)").arg(firstId)), "the second profile disabled a command it does not own");
        QVERIFY2(callRefused(mpSecondHost, qsl("setCommandTooltip(%1, 'hijacked')").arg(firstId)), "the second profile retitled a command it does not own");
        QVERIFY2(callRefused(mpSecondHost, qsl("setCommandChecked(%1, true)").arg(firstId)), "the second profile checked a command it does not own");
        QVERIFY2(callRefused(mpSecondHost, qsl("removeCommand(%1)").arg(firstId)), "the second profile removed a command it does not own");
        QVERIFY2(callRefused(mpFirstHost, qsl("removeCommand(%1)").arg(secondId)), "the first profile removed the second profile's command");
        QTest::qWait(100ms);

        QToolButton* pButton = toolbarButtonNamed(qsl("OwnedByFirst"));
        QVERIFY2(pButton, "the first profile's command did not survive the other profile's attempts");
        QVERIFY2(pButton->isEnabled(), "the first profile's command was disabled by the other profile");
        QVERIFY2(menuActionNamed(qsl("OwnedByFirst")), "the first profile's menu entry did not survive");

        QVERIFY(callReturnedTrue(mpFirstHost, qsl("removeCommand(%1)").arg(firstId)));
        QVERIFY(callReturnedTrue(mpSecondHost, qsl("removeCommand(%1)").arg(secondId)));
        QTest::qWait(100ms);
    }

    // Contract point 4: closing a profile takes its commands with it, without
    // the package doing anything. Nothing covered this, and deleting the
    // cleanup entirely left every other case passing.
    void test_closingAProfileTakesItsCommandsWithIt()
    {
        const QString doomedProfile = qsl("AddonControls-Doomed");
        QVERIFY(provisionProfileOnDisk(doomedProfile));
        QVERIFY2(runLua(mpFirstHost, qsl("loadProfile('%1', true)").arg(doomedProfile)).isNull(), "the third profile could not be loaded");
        QTest::qWait(500ms);
        Host* pDoomedHost = hostFor(doomedProfile);
        QVERIFY2(pDoomedHost, "the third profile did not open");

        const int doomedId = addCommand(pDoomedHost, qsl("name = 'GoesWithTheProfile', menuPath = 'Doomed'"));
        QVERIFY2(doomedId > 0, "the third profile's command was not placed");
        QVERIFY(toolbarButtonNamed(qsl("GoesWithTheProfile")));
        QVERIFY(menuActionNamed(qsl("GoesWithTheProfile")));

        // the public path a player takes, which posts closeHost() itself
        mudlet::self()->slot_closeProfileByName(doomedProfile);
        QTest::qWait(1000ms);
        QCoreApplication::processEvents();

        QVERIFY2(!toolbarButtonNamed(qsl("GoesWithTheProfile")), "the closed profile's button is still on the toolbar");
        QVERIFY2(!menuActionNamed(qsl("GoesWithTheProfile")), "the closed profile's menu entry is still in the menu");
        // and its id names nothing now, rather than another profile's command
        QVERIFY2(callRefused(mpFirstHost, qsl("removeCommand(%1)").arg(doomedId)), "the closed profile's id still addresses something");
    }

    // A profile reset closes the Lua state, so every id the package was holding
    // dies there; the commands those ids named have to go too, or a package
    // that places its command from a script adds another on every reset
    void test_resettingAProfileTakesItsCommandsWithIt()
    {
        const int resetId = addCommand(mpSecondHost, qsl("name = 'GoesWithTheReset', menuPath = 'Reset'"));
        QVERIFY2(resetId > 0, "the command was not placed");
        QVERIFY(toolbarButtonNamed(qsl("GoesWithTheReset")));

        QVERIFY2(runLua(mpSecondHost, qsl("resetProfile()")).isNull(), "resetProfile() did not run");
        QTest::qWait(1500ms);
        QCoreApplication::processEvents();

        QVERIFY2(!toolbarButtonNamed(qsl("GoesWithTheReset")), "the reset left the profile's button on the toolbar");
        QVERIFY2(!menuActionNamed(qsl("GoesWithTheReset")), "the reset left the profile's menu entry in the menu");
    }

    // Returning an id for something nobody can see is worse than refusing:
    // #7998 leaves toolBarVisibility at visibleNever on a fresh profile, so a
    // toolbar-only command would be placed where no new user can reach it
    void test_aToolbarOnlyCommandIsRefusedWhileTheToolbarIsHidden()
    {
        const enums::controlsVisibility restore = mudlet::self()->toolBarVisibility();
        mudlet::self()->setToolBarVisibility(enums::visibleNever);

        const QString why = refusalReason(mpFirstHost, qsl("name = 'Invisible', surfaces = 'toolbar'"));
        QVERIFY2(!why.isEmpty(), "a toolbar-only command was accepted while the toolbar was hidden");
        QVERIFY2(why.contains(qsl("Preferences"), Qt::CaseInsensitive) || why.contains(qsl("hidden"), Qt::CaseInsensitive),
                 qPrintable(qsl("the refusal does not say what to do about it: %1").arg(why)));

        // the same command with a menu entry is still reachable, so it stands
        const int bothId = addCommand(mpFirstHost, qsl("name = 'StillReachable', menuPath = 'Hidden'"));
        QVERIFY2(bothId > 0, "a command on both surfaces was refused while the toolbar was hidden");
        QVERIFY(menuActionNamed(qsl("StillReachable")));
        QVERIFY(callReturnedTrue(mpFirstHost, qsl("removeCommand(%1)").arg(bothId)));

        mudlet::self()->setToolBarVisibility(restore);
        QTest::qWait(100ms);
    }

    // A shortcut Qt cannot parse holds Key_unknown rather than nothing, and one
    // already taken disables both itself and Mudlet's own - with the warning
    // going to a console that released builds do not have
    void test_anUnusableShortcutIsRefusedWithAReason()
    {
        const QString typo = refusalReason(mpFirstHost, qsl("name = 'Typo', menuPath = 'Keys', shortcut = 'Ctrl+Shft+T'"));
        QVERIFY2(!typo.isEmpty(), "a shortcut Qt cannot parse was accepted");

        const int takenId = addCommand(mpFirstHost, qsl("name = 'Taken', menuPath = 'Keys', shortcut = 'Ctrl+Alt+K'"));
        QVERIFY2(takenId > 0, "a free shortcut was refused");
        const QString clash = refusalReason(mpFirstHost, qsl("name = 'Clash', menuPath = 'Keys', shortcut = 'Ctrl+Alt+K'"));
        QVERIFY2(!clash.isEmpty(), "a shortcut already taken was accepted, which disables both");
        QVERIFY2(clash.contains(qsl("Taken")), qPrintable(qsl("the refusal does not name what took it: %1").arg(clash)));

        QVERIFY(callReturnedTrue(mpFirstHost, qsl("removeCommand(%1)").arg(takenId)));
        QTest::qWait(100ms);
    }

    // The same clash from the other side: a submenu exists, and a command tries
    // to take its label. The review said the duplicate happened in either
    // order, and the first fix only closed one of them.
    void test_aCommandCannotTakeASubmenusLabel()
    {
        const int nestedId = addCommand(mpFirstHost, qsl("name = 'Alice', menuPath = 'Voices'"));
        QVERIFY2(nestedId > 0, "the nested command was not placed");

        const QString why = refusalReason(mpFirstHost, qsl("name = 'Voices', menuPath = ''"));
        QVERIFY2(!why.isEmpty(), "a command took the label of an existing submenu, which shows it twice");
        QVERIFY2(why.contains(qsl("Voices")), qPrintable(qsl("the refusal does not name the clash: %1").arg(why)));

        // two commands sharing a label stay legal - ids are the identity
        const int firstTwin = addCommand(mpFirstHost, qsl("name = 'Twin', menuPath = 'Twins'"));
        const int secondTwin = addCommand(mpFirstHost, qsl("name = 'Twin', menuPath = 'Twins'"));
        QVERIFY2(firstTwin > 0 && secondTwin > 0 && firstTwin != secondTwin, "two commands with one label were refused");

        QVERIFY(callReturnedTrue(mpFirstHost, qsl("removeCommand(%1)").arg(nestedId)));
        QVERIFY(callReturnedTrue(mpFirstHost, qsl("removeCommand(%1)").arg(firstTwin)));
        QVERIFY(callReturnedTrue(mpFirstHost, qsl("removeCommand(%1)").arg(secondTwin)));
        QTest::qWait(100ms);
    }

    // A menuPath part naming an existing command would put two entries with one
    // label in the menu, one a command and one a submenu
    void test_aMenuPathThatNamesACommandIsRefused()
    {
        const int leafId = addCommand(mpFirstHost, qsl("name = 'Speech', menuPath = ''"));
        QVERIFY2(leafId > 0, "the leaf command was not placed");

        const QString why = refusalReason(mpFirstHost, qsl("name = 'Alice', menuPath = 'Speech/Voices'"));
        QVERIFY2(!why.isEmpty(), "a menuPath naming an existing command was accepted, which duplicates the label");
        QVERIFY2(why.contains(qsl("Speech")), qPrintable(qsl("the refusal does not name the clash: %1").arg(why)));

        QVERIFY(callReturnedTrue(mpFirstHost, qsl("removeCommand(%1)").arg(leafId)));
        QTest::qWait(100ms);
    }
};

#include "AddonControlsTest.moc"
MUDLET_GROUPED_TEST_MAIN(AddonControlsTest)
