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
 * The addon toolbar and menu API of docs/addon-ui-api.md is called from Lua,
 * but the three things held here are not reachable from a spec: a click has to
 * come from a real QToolButton or QAction, the submenu pruning is Qt object
 * lifetime across an event loop turn, and ownership means nothing until a
 * second profile is open.
 *
 * What this holds:
 *  - the click events carry the id as a string, which is what the API document
 *    promises handlers;
 *  - removing the last item of a menuPath and adding another at that same path
 *    in one turn - what a sysMenuItemClicked handler does - leaves the new item
 *    alive once the deferred deletions have run;
 *  - a profile cannot remove or alter a control another profile created, even
 *    though the ids are handed out from one application-wide sequence.
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

    // The id a Lua call returned, or -1 when the call refused
    int addToolbarButton(Host* pHost, const QString& name) const
    {
        const QString error = runLua(pHost, qsl("_addonId = addToolbarButton('%1', '', 'a tooltip') or -1").arg(name));
        if (!error.isNull()) {
            qWarning() << "addToolbarButton:" << error;
            return -1;
        }
        return luaGlobalNumber(pHost, qsl("_addonId"));
    }

    int addMenuItem(Host* pHost, const QString& menuPath, const QString& name) const
    {
        const QString error = runLua(pHost, qsl("_addonId = addMenuItem('%1', '%2') or -1").arg(menuPath, name));
        if (!error.isNull()) {
            qWarning() << "addMenuItem:" << error;
            return -1;
        }
        return luaGlobalNumber(pHost, qsl("_addonId"));
    }

    // Whether a boolean-returning binding said yes
    bool callReturnedTrue(Host* pHost, const QString& call) const
    {
        const QString error = runLua(pHost, qsl("_addonResult = %1").arg(call));
        if (!error.isNull()) {
            qWarning() << "callReturnedTrue:" << error;
            return false;
        }
        return luaGlobalBoolean(pHost, qsl("_addonResult"));
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

    // docs/addon-ui-api.md gives both click events an id "(as string)", and a
    // handler written to that contract does string things with it
    void test_aClickHandsTheHandlerTheIdAsAString()
    {
        QVERIFY2(runLua(mpFirstHost,
                        qsl("_addonSeenType, _addonSeenId = nil, nil\n"
                            "function _addonClickHandler(event, id)\n"
                            "  _addonSeenType = type(id)\n"
                            "  _addonSeenId = tostring(id)\n"
                            "end\n"
                            "registerAnonymousEventHandler('sysToolbarButtonClicked', '_addonClickHandler')\n"
                            "registerAnonymousEventHandler('sysMenuItemClicked', '_addonClickHandler')"))
                         .isNull(),
                 "the click handler could not be registered");

        const int buttonId = addToolbarButton(mpFirstHost, qsl("ClickedButton"));
        QVERIFY2(buttonId > 0, "the toolbar button was not added");
        QToolButton* pButton = toolbarButtonNamed(qsl("ClickedButton"));
        QVERIFY2(pButton, "the toolbar button is not on the toolbar");

        pButton->click();
        QTest::qWait(100ms);
        QCOMPARE(luaGlobalString(mpFirstHost, qsl("_addonSeenType")), qsl("string"));
        QCOMPARE(luaGlobalString(mpFirstHost, qsl("_addonSeenId")), QString::number(buttonId));

        QVERIFY(runLua(mpFirstHost, qsl("_addonSeenType, _addonSeenId = nil, nil")).isNull());

        const int itemId = addMenuItem(mpFirstHost, qsl("ClickTest"), qsl("ClickedItem"));
        QVERIFY2(itemId > 0, "the menu item was not added");
        QAction* pAction = menuActionNamed(qsl("ClickedItem"));
        QVERIFY2(pAction, "the menu item is not in the menu");

        pAction->trigger();
        QTest::qWait(100ms);
        QCOMPARE(luaGlobalString(mpFirstHost, qsl("_addonSeenType")), qsl("string"));
        QCOMPARE(luaGlobalString(mpFirstHost, qsl("_addonSeenId")), QString::number(itemId));

        QVERIFY(callReturnedTrue(mpFirstHost, qsl("removeToolbarButton(%1)").arg(buttonId)));
        QVERIFY(callReturnedTrue(mpFirstHost, qsl("removeMenuItem(%1)").arg(itemId)));
        QTest::qWait(100ms);
    }

    // A sysMenuItemClicked handler that removes the item it was called for and
    // puts another at the same path runs entirely inside one turn of the event
    // loop, so the add sees a submenu the remove has already condemned
    void test_aPathReusedInTheSameTurnKeepsItsNewItem()
    {
        QVERIFY2(runLua(mpFirstHost,
                        qsl("_addonFirstId = addMenuItem('Reuse/Nested', 'FirstItem')\n"
                            "_addonRemoved = removeMenuItem(_addonFirstId)\n"
                            "_addonSecondId = addMenuItem('Reuse/Nested', 'SecondItem')"))
                         .isNull(),
                 "the remove-then-add chunk did not run");
        QVERIFY(luaGlobalBoolean(mpFirstHost, qsl("_addonRemoved")));
        const int secondId = luaGlobalNumber(mpFirstHost, qsl("_addonSecondId"));
        QVERIFY2(secondId > 0, "the second menu item was not added");

        // the deletions the removal deferred happen here
        QTest::qWait(200ms);
        QCoreApplication::processEvents();

        QVERIFY2(menuActionNamed(qsl("SecondItem")), "the item added at the reused path is gone once the deferred deletions have run");
        QVERIFY2(callReturnedTrue(mpFirstHost, qsl("setMenuItemEnabled(%1, false)").arg(secondId)), "the reused path's item no longer answers to its own id");

        QVERIFY(callReturnedTrue(mpFirstHost, qsl("removeMenuItem(%1)").arg(secondId)));
        QTest::qWait(100ms);
    }

    // Ids come from one application-wide sequence, so a second profile can name
    // the first profile's controls whether or not it created them
    void test_aProfileCannotTouchAnotherProfilesControls()
    {
        const int buttonId = addToolbarButton(mpFirstHost, qsl("OwnedButton"));
        QVERIFY2(buttonId > 0, "the first profile's toolbar button was not added");
        const int itemId = addMenuItem(mpFirstHost, qsl("Owned"), qsl("OwnedItem"));
        QVERIFY2(itemId > 0, "the first profile's menu item was not added");

        QVERIFY2(!callReturnedTrue(mpSecondHost, qsl("setToolbarButtonEnabled(%1, false)").arg(buttonId)), "the second profile disabled a button it does not own");
        QVERIFY2(!callReturnedTrue(mpSecondHost, qsl("setToolbarButtonTooltip(%1, 'hijacked')").arg(buttonId)), "the second profile retitled a button it does not own");
        QVERIFY2(!callReturnedTrue(mpSecondHost, qsl("removeToolbarButton(%1)").arg(buttonId)), "the second profile removed a button it does not own");
        QVERIFY2(!callReturnedTrue(mpSecondHost, qsl("setMenuItemChecked(%1, true)").arg(itemId)), "the second profile checked a menu item it does not own");
        QVERIFY2(!callReturnedTrue(mpSecondHost, qsl("removeMenuItem(%1)").arg(itemId)), "the second profile removed a menu item it does not own");
        QTest::qWait(100ms);

        QToolButton* pButton = toolbarButtonNamed(qsl("OwnedButton"));
        QVERIFY2(pButton, "the first profile's button did not survive the second profile's attempts");
        QVERIFY2(pButton->isEnabled(), "the first profile's button was disabled by the other profile");
        QVERIFY2(menuActionNamed(qsl("OwnedItem")), "the first profile's menu item did not survive the second profile's attempts");

        // and the owner is still served
        QVERIFY(callReturnedTrue(mpFirstHost, qsl("setToolbarButtonEnabled(%1, false)").arg(buttonId)));
        QVERIFY(callReturnedTrue(mpFirstHost, qsl("removeToolbarButton(%1)").arg(buttonId)));
        QVERIFY(callReturnedTrue(mpFirstHost, qsl("removeMenuItem(%1)").arg(itemId)));
        QTest::qWait(100ms);
    }
};

#include "AddonControlsTest.moc"
MUDLET_GROUPED_TEST_MAIN(AddonControlsTest)
