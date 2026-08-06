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

/*
 * The profile tab switching shortcuts (Ctrl+1 to Ctrl+9, Ctrl+Tab) are
 * application-wide QShortcuts on the main window. Qt resolves a QShortcut by
 * first sending a QEvent::ShortcutOverride to the focus widget and, unless that
 * widget accepts it, running the shortcut and never delivering the KeyPress.
 * A user's own key binding on one of those keys therefore only survives if the
 * command line claims the override - which is what these tests check, along
 * with the two ways it must NOT claim it: no binding, and a binding that is
 * disabled (directly or through its group).
 *
 * Run with: ctest -R ProfileSwitchShortcutTest -V
 */

#include <QtTest/QtTest>
#include <chrono>

#include "Host.h"
#include "KeyUnit.h"
#include "MudletInstanceCoordinator.h"
#include "TCommandLine.h"
#include "TKey.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include <QShortcut>

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

using namespace std::chrono_literals;

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForProfileSwitchShortcutTest();

// Qt::CTRL is the Cmd key on macOS, where the "next profile" shortcut uses the
// physical Control key (Qt::META) instead - mirrors mudlet::mudlet():
#if defined(Q_OS_MACOS)
static constexpr Qt::KeyboardModifier nextProfileModifier = Qt::MetaModifier;
#else
static constexpr Qt::KeyboardModifier nextProfileModifier = Qt::ControlModifier;
#endif

class ProfileSwitchShortcutTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "ProfileSwitchShortcut-Test";
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = "localhost";

    TCommandLine* commandLine() const
    {
        if (!mpHost || !mpHost->mpConsole) {
            return nullptr;
        }
        return mpHost->mpConsole->mpCommandLine;
    }

    // Replays what QShortcutMap does before it runs a shortcut: it offers the
    // key to the focus widget as an ignored ShortcutOverride and only runs the
    // shortcut if nobody accepted it.
    bool overrideClaimed(int key, Qt::KeyboardModifiers modifiers) const
    {
        QKeyEvent event(QEvent::ShortcutOverride, key, modifiers);
        event.ignore();
        QApplication::sendEvent(commandLine(), &event);
        return event.isAccepted();
    }

    void sendKeyPress(int key, Qt::KeyboardModifiers modifiers) const
    {
        QKeyEvent event(QEvent::KeyPress, key, modifiers);
        QApplication::sendEvent(commandLine(), &event);
    }

    // The shortcuts are only worth overriding if they are actually installed,
    // so every test that asserts a claim also proves the collision is real.
    bool shortcutInstalledFor(const QKeySequence& sequence) const
    {
        const auto shortcuts = mudlet::self()->findChildren<QShortcut*>();
        for (auto* shortcut : shortcuts) {
            if (shortcut->key() == sequence && shortcut->isEnabled()) {
                return true;
            }
        }
        return false;
    }

    int luaCounter(const QString& globalName) const
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        lua_getglobal(L, globalName.toUtf8().constData());
        const int value = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        return value;
    }

    // Creates a permanent key binding whose script counts its own invocations
    // into a Lua global, and returns its id.
    int createCountingKey(const QString& name, int keycode, int modifier, const QString& counterName, const QString& parent = QString())
    {
        QString keyName = name;
        QString parentName = parent;
        QString script = qsl("%1 = (%1 or 0) + 1").arg(counterName);
        auto [id, message] = mpHost->mLuaInterpreter.startPermKey(keyName, parentName, keycode, modifier, script);
        if (id <= 0) {
            qWarning() << "createCountingKey failed:" << message;
        }
        return id;
    }

    void removeAllKeys()
    {
        auto* keyUnit = mpHost->getKeyUnit();
        const auto rootKeys = keyUnit->getKeyRootNodeList();
        for (auto* key : rootKeys) {
            delete key;
        }
    }

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForProfileSwitchShortcutTest();

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);

        startProfile(mHostname, mLocalhost, mPort);
        mpHost = mudlet::self()->getActiveHost();
        QVERIFY2(mpHost, "No active host after profile creation");
        QVERIFY2(commandLine(), "No command line available for the test");
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

    void cleanup() { removeAllKeys(); }

    // The regression itself: a user binding on Ctrl+1 must beat "Switch to
    // profile 1".
    void test_userBindingOnCtrlNumberClaimsTheShortcut()
    {
        QVERIFY2(shortcutInstalledFor(QKeySequence(Qt::CTRL | Qt::Key_1)), "No profile switching shortcut is installed for Ctrl+1, so this test proves nothing");

        QVERIFY(createCountingKey(qsl("Ctrl+1 binding"), Qt::Key_1, Qt::ControlModifier, qsl("_testCtrl1")) > 0);

        QVERIFY2(overrideClaimed(Qt::Key_1, Qt::ControlModifier), "A user key binding on Ctrl+1 did not claim the key, so the profile switch shortcut swallows it");
    }

    // Without a binding the key has to be left alone so the tab switch happens.
    void test_withoutUserBindingTheShortcutKeepsTheKey()
    {
        QVERIFY2(!overrideClaimed(Qt::Key_1, Qt::ControlModifier), "Ctrl+1 was claimed even though no user key binding matches it - profile switching would stop working");
    }

    // A binding on a key that no profile switching shortcut uses needs no
    // claim - Ctrl+0 is the control case that always worked, because only nine
    // shortcuts (Ctrl+1 to Ctrl+9) are installed.
    void test_bindingOnAnUnshadowedKeyIsNotClaimed()
    {
        QVERIFY(createCountingKey(qsl("Ctrl+0 binding"), Qt::Key_0, Qt::ControlModifier, qsl("_testCtrl0")) > 0);

        QVERIFY2(!overrideClaimed(Qt::Key_0, Qt::ControlModifier), "Ctrl+0 is not a profile switching shortcut, so the command line must not claim it");
    }

    // A disabled binding is not a binding, so it must not steal the key from
    // the tab switch.
    void test_disabledUserBindingDoesNotClaimTheShortcut()
    {
        const int id = createCountingKey(qsl("Disabled Ctrl+2 binding"), Qt::Key_2, Qt::ControlModifier, qsl("_testCtrl2"));
        QVERIFY(id > 0);
        QVERIFY(overrideClaimed(Qt::Key_2, Qt::ControlModifier));

        auto* key = mpHost->getKeyUnit()->getKey(id);
        QVERIFY(key);
        key->setIsActive(false);

        QVERIFY2(!overrideClaimed(Qt::Key_2, Qt::ControlModifier), "A disabled key binding still claimed Ctrl+2");
    }

    // Same for an active binding sitting inside a disabled group.
    void test_userBindingInDisabledGroupDoesNotClaimTheShortcut()
    {
        const int groupId = createCountingKey(qsl("Key Group"), -1, 0, qsl("_testGroup"));
        QVERIFY(groupId > 0);
        auto* group = mpHost->getKeyUnit()->getKey(groupId);
        QVERIFY(group);
        group->setIsActive(true);

        QVERIFY(createCountingKey(qsl("Grouped Ctrl+3 binding"), Qt::Key_3, Qt::ControlModifier, qsl("_testCtrl3"), qsl("Key Group")) > 0);
        QVERIFY2(overrideClaimed(Qt::Key_3, Qt::ControlModifier), "A binding in an enabled group should claim Ctrl+3");

        group->setIsActive(false);
        QVERIFY2(!overrideClaimed(Qt::Key_3, Qt::ControlModifier), "A binding inside a disabled group still claimed Ctrl+3");
    }

    // Ctrl+Tab ("Next profile") is shadowed the same way and needs the same
    // treatment, in both directions.
    void test_userBindingOnCtrlTabClaimsTheShortcut()
    {
        QVERIFY2(!overrideClaimed(Qt::Key_Tab, nextProfileModifier), "Ctrl+Tab was claimed with no user key binding present");

        QVERIFY(createCountingKey(qsl("Ctrl+Tab binding"), Qt::Key_Tab, nextProfileModifier, qsl("_testCtrlTab")) > 0);

        QVERIFY2(overrideClaimed(Qt::Key_Tab, nextProfileModifier), "A user key binding on Ctrl+Tab did not claim the key");
    }

    // Claiming the override only defers the key - the binding must then run
    // exactly once, off the KeyPress that the claim let through, and not a
    // second time from the probe itself.
    void test_claimedBindingRunsExactlyOnce()
    {
        QVERIFY(createCountingKey(qsl("Ctrl+4 binding"), Qt::Key_4, Qt::ControlModifier, qsl("_testCtrl4")) > 0);
        QCOMPARE(luaCounter(qsl("_testCtrl4")), 0);

        QVERIFY(overrideClaimed(Qt::Key_4, Qt::ControlModifier));
        QCOMPARE(luaCounter(qsl("_testCtrl4")), 0); // the probe must not execute anything

        sendKeyPress(Qt::Key_4, Qt::ControlModifier);
        QCOMPARE(luaCounter(qsl("_testCtrl4")), 1);
    }

private:
    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        QTimer::singleShot(0ms, qApp, [hostname, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100ms);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), hostname);
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
        if (!spy.wait(5000)) {
            QFAIL("Profile took too long to load.");
        }
        auto host = mudlet::self()->getActiveHost();
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy connectionSpy(&(host->mTelnet), &cTelnet::signal_connected);
        if (!connectionSpy.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);

        if (!dir.exists()) {
            return;
        }
        dir.removeRecursively();
    }
};

void initializeQRCResourcesForProfileSwitchShortcutTest()
{
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
    qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
    qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    qInitResources_mudlet_fonts_posix();
#endif
#endif
    qInitResources_mudlet();
    qInitResources_qm();
}

#include "ProfileSwitchShortcutTest.moc"
QTEST_MAIN(ProfileSwitchShortcutTest)
