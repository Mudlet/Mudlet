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
 * The command line must claim the ShortcutOverride exactly when a live user key
 * binding matches a press that would otherwise activate a profile switching
 * shortcut (Ctrl+1 to Ctrl+9, Ctrl+Tab) - including presses QShortcutMap
 * matches to a differently spelt shortcut - and never otherwise. Claiming it is
 * the only way the binding survives, since QShortcutMap otherwise runs the
 * shortcut and never delivers the KeyPress.
 *
 * Run with: ctest -R ProfileSwitchShortcutTest -V
 */

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "ProfileTestHelper.h"
#include "Host.h"
#include "KeyUnit.h"
#include "MudletInstanceCoordinator.h"
#include "TCommandLine.h"
#include "TKey.h"
#include "TLuaInterpreter.h"
#include "ShortcutsManager.h"
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

#include "GroupedTest.h"

using namespace std::chrono_literals;

// Qt::CTRL is Cmd on macOS, where "next profile" uses Qt::META - see mudlet::mudlet()
#if defined(Q_OS_MACOS)
static constexpr Qt::KeyboardModifier nextProfileModifier = Qt::MetaModifier;
#else
static constexpr Qt::KeyboardModifier nextProfileModifier = Qt::ControlModifier;
#endif

class ProfileSwitchShortcutTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
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

    // QShortcutMap offers the key as an ignored ShortcutOverride and only runs
    // the shortcut if nobody accepted it
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

    // Asserted where a claim is expected, so a mis-mapped sequence cannot
    // masquerade as a code failure
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

    // Alt+E on Linux and Windows, Ctrl+E on macOS
    std::pair<int, Qt::KeyboardModifiers> scriptEditorShortcut() const
    {
        auto* sequence = mudlet::self()->shortcutsManager()->getSequence(qsl("Script editor"));
        if (!sequence || sequence->isEmpty()) {
            return {Qt::Key_unknown, Qt::NoModifier};
        }
        const QKeyCombination combination = (*sequence)[0];
        return {combination.key(), combination.keyboardModifiers()};
    }

    int luaCounter(const QString& globalName) const
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        lua_getglobal(L, globalName.toUtf8().constData());
        const int value = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        return value;
    }

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

    // Safe only while every key here is permanent and none is killed or
    // uninstalled, leaving KeyUnit's deferred-delete set empty; otherwise this
    // has to go through markCleanup()/doCleanup()
    void removeAllKeys()
    {
        auto* keyUnit = mpHost->getKeyUnit();
        const auto rootKeys = keyUnit->getKeyRootNodeList(); // by value: ~TKey mutates the real list
        for (auto* key : rootKeys) {
            delete key;
        }
    }

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own. Sharing the developer's
        // ~/.config/mudlet means sharing a profile list, so a second copy of
        // this test running at the same time is told the name it types is
        // already in use and never gets an enabled Connect button. Since #9712
        // the opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);

        startProfile(mHostname, mLocalhost, mPort);
        mpHost = mudlet::self()->getActiveHost();
        QVERIFY2(mpHost, "No active host after profile creation");
        QVERIFY2(commandLine(), "No command line available for the test");

        // Checked ahead of the claim in TCommandLine::event(), so CtrlTab here
        // would take Ctrl+Tab out of these tests' hands
        mpHost->mCaretShortcut = Host::CaretShortcut::None;
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            deleteProfileDirectory(mHostname);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void cleanup() { removeAllKeys(); }

    void test_userBindingOnCtrlNumberClaimsTheShortcut()
    {
        QVERIFY2(shortcutInstalledFor(QKeySequence(Qt::CTRL | Qt::Key_1)), "No profile switching shortcut is installed for Ctrl+1, so this test proves nothing");

        QVERIFY(createCountingKey(qsl("Ctrl+1 binding"), Qt::Key_1, Qt::ControlModifier, qsl("_testCtrl1")) > 0);

        QVERIFY2(overrideClaimed(Qt::Key_1, Qt::ControlModifier), "A user key binding on Ctrl+1 did not claim the key, so the profile switch shortcut swallows it");
    }

    void test_withoutUserBindingTheShortcutKeepsTheKey()
    {
        QVERIFY2(!overrideClaimed(Qt::Key_1, Qt::ControlModifier), "Ctrl+1 was claimed even though no user key binding matches it - profile switching would stop working");
    }

    // Only nine shortcuts are installed, so Ctrl+0 has nothing to beat
    void test_bindingOnAnUnshadowedKeyIsNotClaimed()
    {
        QVERIFY(createCountingKey(qsl("Ctrl+0 binding"), Qt::Key_0, Qt::ControlModifier, qsl("_testCtrl0")) > 0);

        QVERIFY2(!overrideClaimed(Qt::Key_0, Qt::ControlModifier), "Ctrl+0 is not a profile switching shortcut, so the command line must not claim it");
    }

    void test_disabledUserBindingDoesNotClaimTheShortcut()
    {
        const QString name = qsl("Disabled Ctrl+2 binding");
        QVERIFY(createCountingKey(name, Qt::Key_2, Qt::ControlModifier, qsl("_testCtrl2")) > 0);
        QVERIFY(overrideClaimed(Qt::Key_2, Qt::ControlModifier));

        QVERIFY(mpHost->getKeyUnit()->disableKey(name));

        QVERIFY2(!overrideClaimed(Qt::Key_2, Qt::ControlModifier), "A disabled key binding still claimed Ctrl+2");
    }

    void test_userBindingOnCtrlNineClaimsTheShortcut()
    {
        QVERIFY2(shortcutInstalledFor(QKeySequence(Qt::CTRL | Qt::Key_9)), "No profile switching shortcut is installed for Ctrl+9, so this test proves nothing");

        QVERIFY(createCountingKey(qsl("Ctrl+9 binding"), Qt::Key_9, Qt::ControlModifier, qsl("_testCtrl9")) > 0);

        QVERIFY2(overrideClaimed(Qt::Key_9, Qt::ControlModifier), "A user key binding on Ctrl+9 did not claim the key");
    }

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

    void test_userBindingOnCtrlTabClaimsTheShortcut()
    {
        QVERIFY2(shortcutInstalledFor(QKeySequence(nextProfileModifier | Qt::Key_Tab)), "No 'Next profile' shortcut is installed for Ctrl+Tab, so this test proves nothing");
        QVERIFY2(!overrideClaimed(Qt::Key_Tab, nextProfileModifier), "Ctrl+Tab was claimed with no user key binding present");

        QVERIFY(createCountingKey(qsl("Ctrl+Tab binding"), Qt::Key_Tab, nextProfileModifier, qsl("_testCtrlTab")) > 0);

        QVERIFY2(overrideClaimed(Qt::Key_Tab, nextProfileModifier), "A user key binding on Ctrl+Tab did not claim the key");
    }

    // Shift+Tab reaches the widget as Key_Backtab while the sequence is spelt
    // with Key_Tab, so the match has to bridge the two spellings
    void test_userBindingOnCtrlShiftTabClaimsTheShortcut()
    {
        const auto modifiers = nextProfileModifier | Qt::ShiftModifier;
        QVERIFY2(!overrideClaimed(Qt::Key_Backtab, modifiers), "Ctrl+Shift+Tab was claimed with no user key binding present");

        QVERIFY(createCountingKey(qsl("Ctrl+Shift+Tab binding"), Qt::Key_Backtab, modifiers, qsl("_testCtrlShiftTab")) > 0);

        QVERIFY2(overrideClaimed(Qt::Key_Backtab, modifiers), "A user key binding on Ctrl+Shift+Tab did not claim the key");
    }

    void test_bindingOnAnotherApplicationShortcutIsNotClaimed()
    {
        auto [key, modifiers] = scriptEditorShortcut();
        QVERIFY2(key != Qt::Key_unknown, "Could not read the script editor shortcut");

        QVERIFY(createCountingKey(qsl("Script editor shortcut binding"), key, modifiers, qsl("_testEditor")) > 0);

        QVERIFY2(!overrideClaimed(key, modifiers), "A key binding claimed the script editor shortcut, which is outside the profile switching set");
    }

    // These keys build non-empty candidate sequences, which cannot compare equal
    // to a cleared shortcut whether or not profileSwitchShortcutMatches() guards
    // against empty ones - the press that can is covered by
    // test_aClearedProfileShortcutDoesNotMatchAnUnnamedKeyPress()
    void test_aClearedProfileShortcutDoesNotClaimEveryKey()
    {
        auto [key, modifiers] = scriptEditorShortcut();
        QVERIFY(createCountingKey(qsl("Script editor shortcut binding"), key, modifiers, qsl("_testEditorCleared")) > 0);
        QVERIFY(createCountingKey(qsl("F5 binding"), Qt::Key_F5, Qt::NoModifier, qsl("_testF5")) > 0);

        auto* sequence = mudlet::self()->shortcutsManager()->getSequence(qsl("Switch to profile 1"));
        QVERIFY2(sequence, "'Switch to profile 1' is not registered with the shortcuts manager");
        const QKeySequence saved = *sequence;
        *sequence = QKeySequence();

        const bool editorClaimed = overrideClaimed(key, modifiers);
        const bool f5Claimed = overrideClaimed(Qt::Key_F5, Qt::NoModifier);
        *sequence = saved;

        QVERIFY2(!editorClaimed, "A cleared profile switching shortcut made an unrelated bound key claim the override");
        QVERIFY2(!f5Claimed, "A cleared profile switching shortcut made an unrelated bound key claim the override");
    }

    // Qt spells a press it cannot name as either key() == 0 or Qt::Key_unknown,
    // and only the 0 spelling builds an empty candidate sequence. A shortcut
    // cleared in the preferences is empty too, and two empty sequences compare
    // equal, so for as long as any one profile switching shortcut is cleared an
    // unnamed press matches it unless profileSwitchShortcutMatches() rejects
    // empty sequences. Every modifier that function strips reaches that point,
    // so a guard applied to only some of the candidates is caught too.
    //
    // Asserted on profileSwitchShortcutMatches() rather than through
    // overrideClaimed() because QPlainTextEdit's own ShortcutOverride handling
    // (QWidgetTextControl) accepts any unmodified key below Qt::Key_Escape as a
    // text editing shortcut, so the command line claims an unnamed press either
    // way and the end-to-end path cannot tell the two apart.
    void test_aClearedProfileShortcutDoesNotMatchAnUnnamedKeyPress()
    {
        constexpr int unnamedKey = 0;

        auto* sequence = mudlet::self()->shortcutsManager()->getSequence(qsl("Switch to profile 1"));
        QVERIFY2(sequence, "'Switch to profile 1' is not registered with the shortcuts manager");
        const QKeySequence saved = *sequence;
        *sequence = QKeySequence();

        const bool clearedShortcutIsEmpty = sequence->isEmpty();
        QStringList matchedModifiers;
        for (const auto modifiers :
             {Qt::KeyboardModifiers(Qt::NoModifier), Qt::KeyboardModifiers(Qt::KeypadModifier), Qt::KeyboardModifiers(Qt::ShiftModifier), Qt::ShiftModifier | Qt::KeypadModifier}) {
            const QKeyEvent unnamedPress(QEvent::ShortcutOverride, unnamedKey, modifiers);
            if (mudlet::self()->profileSwitchShortcutMatches(&unnamedPress)) {
                matchedModifiers << qsl("0x%1").arg(modifiers.toInt(), 0, 16);
            }
        }
        *sequence = saved;

        QVERIFY2(clearedShortcutIsEmpty, "Clearing the shortcut did not leave it empty, so this test proves nothing");
        QVERIFY2(QKeySequence(QKeyCombination(Qt::NoModifier, static_cast<Qt::Key>(unnamedKey))).isEmpty(), "An unnamed key no longer builds an empty sequence, so this test proves nothing");
        QVERIFY2(matchedModifiers.isEmpty(), qPrintable(qsl("A cleared profile switching shortcut matched a key press Qt could not name, with modifiers %1").arg(matchedModifiers.join(qsl(", ")))));
    }

    // QShortcutMap retries with the keypad modifier stripped, so Ctrl and a
    // numpad digit activates the plain Ctrl+1 shortcut
    void test_userBindingOnAKeypadDigitClaimsTheShortcut()
    {
        const auto modifiers = Qt::ControlModifier | Qt::KeypadModifier;
        QVERIFY(createCountingKey(qsl("Ctrl+keypad 1 binding"), Qt::Key_1, modifiers, qsl("_testKeypad1")) > 0);

        QVERIFY2(overrideClaimed(Qt::Key_1, modifiers), "A user key binding on Ctrl and a keypad digit did not claim the key");
    }

    // Layouts needing Shift for a top-row digit (French AZERTY) record the
    // binding with Shift, and QShortcutMap drops the Shift it consumed
    void test_userBindingOnAShiftedDigitClaimsTheShortcut()
    {
        const auto modifiers = Qt::ControlModifier | Qt::ShiftModifier;
        QVERIFY(createCountingKey(qsl("Ctrl+Shift+1 binding"), Qt::Key_1, modifiers, qsl("_testShift1")) > 0);

        QVERIFY2(overrideClaimed(Qt::Key_1, modifiers), "A user key binding on Ctrl+Shift and a digit did not claim the key");
    }

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
        auto host = TestProfile::create(hostname, address, port);
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

#include "ProfileSwitchShortcutTest.moc"
MUDLET_GROUPED_TEST_MAIN(ProfileSwitchShortcutTest)
