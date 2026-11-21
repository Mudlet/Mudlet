/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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
 * The commands and buttons scripts add to the GUI's main window, as THeadlessSession gives them to an app that
 * embeds Mudlet. Each test makes a session of its own, and with it the process's one profile pool.
 *
 * Run with: ctest -R HeadlessSessionCommandsTest -V
 */

#include <QDir>
#include <QFile>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <cstring>
#include <memory>
#include <optional>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "ActionUnit.h"
#include "TAction.h"
#include "PortableModeTestHelper.h"
#include "THeadlessSession.h"
#include "TLuaInterpreter.h"

#include "GroupedTest.h"

class HeadlessSessionCommandsTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    static bool writeFile(const QString& path, const QByteArray& contents)
    {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
    }

    static bool writeProfile(const QString& name)
    {
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        return QDir().mkpath(saveFolder) && writeFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder), R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<HostPackage><Host></Host></HostPackage>
</MudletPackage>
)");
    }

    static QByteArray packageXml(const QByteArray& script)
    {
        return R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<ScriptPackage><Script isActive="yes" isFolder="no"><name>fromPackage</name><packageName></packageName><script>)"
               + script + R"(</script><eventHandlerList/></Script></ScriptPackage>
</MudletPackage>
)";
    }

    static QString luaGlobalString(Host* host, const char* name)
    {
        lua_State* L = host->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
    }

    static QStringList names(const QList<THeadlessSession::AddonCommand>& commands)
    {
        QStringList result;
        for (const auto& command : commands) {
            result << command.name;
        }
        return result;
    }

    static TAction* makeAction(Host* host, TAction* parent, const QString& name, const bool folder = false, const bool active = true)
    {
        auto* action = parent ? new TAction(parent, host) : new TAction(name, host);
        action->setName(name);
        action->setIsFolder(folder);
        action->setIsActive(active);
        host->getActionUnit()->registerAction(action);
        return action;
    }

    static QStringList buttonNames(const QList<THeadlessSession::Button>& buttons)
    {
        QStringList result;
        for (const auto& button : buttons) {
            result << button.name;
        }
        return result;
    }

    static QStringList barNames(const QList<THeadlessSession::ButtonBar>& bars)
    {
        QStringList result;
        for (const auto& bar : bars) {
            result << bar.name;
        }
        return result;
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
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        QVERIFY2(!HostManager::self(), "A profile pool already exists, so a session cannot make its own.");
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void test_theShortcutsTheAppKeepsAreRefused()
    {
        const QString name = qsl("Headless Commands Reserved");
        QVERIFY(writeProfile(name));
        const auto forgetReserved = qScopeGuard([]() {
            THeadlessSession::setReservedShortcuts({});
        });
        THeadlessSession::setReservedShortcuts({{QKeySequence(qsl("Ctrl+Alt+L")), qsl("Toggle logging")}, {QKeySequence(), qsl("No key")}});
        QCOMPARE(THeadlessSession::reservedShortcuts(), (QList<THeadlessSession::ReservedShortcut>{{QKeySequence(qsl("Ctrl+Alt+L")), qsl("Toggle logging")}}));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
headlessKept, headlessWhy = addCommand({name = "Kept", shortcut = "ctrl+alt+l"})
headlessWhy = tostring(headlessKept) .. " " .. tostring(headlessWhy)
headlessFree = addCommand({name = "Free", shortcut = "Ctrl+Alt+K"})
headlessToolbar = type(addCommand({name = "Toolbar", surfaces = "toolbar"}))
)lua")));
        QCOMPARE(luaGlobalString(session.host(), "headlessWhy"), qsl("nil %1 is already taken by \"Toggle logging\"").arg(QKeySequence(qsl("Ctrl+Alt+L")).toString(QKeySequence::NativeText)));
        QCOMPARE(luaGlobalString(session.host(), "headlessToolbar"), qsl("number"));
        QCOMPARE(names(session.addonCommands()), (QStringList{qsl("Free"), qsl("Toolbar")}));

        // Once given up, the key is handed out, and the command that has it by then keeps it
        THeadlessSession::setReservedShortcuts({{QKeySequence(qsl("Ctrl+Alt+K")), qsl("Search")}});
        QVERIFY(session.runLua(qsl(R"lua(headlessLater = type(addCommand({name = "Later", shortcut = "Ctrl+Alt+L"})))lua")));
        QCOMPARE(luaGlobalString(session.host(), "headlessLater"), qsl("number"));
        QCOMPARE(session.pressKey(Qt::Key_K, Qt::ControlModifier | Qt::AltModifier), THeadlessSession::KeyResult::commandRan);
    }

    void test_theCommandsPackagesAddAreListed()
    {
        const QString name = qsl("Headless Commands Listed");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QVERIFY(session.addonCommands().isEmpty());
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.addonCommands().isEmpty());
        QVERIFY(session.runLua(qsl(R"lua(
headlessSpeech = addCommand({name = "Speech", icon = "/no/such/icon.png", tooltip = "Say it", menuPath = "Speech/Voices", shortcut = "Ctrl+Alt+K"})
headlessFlash = addCommand({name = "Flash", surfaces = "toolbar"})
headlessMenu = addCommand({name = "Menu only", surfaces = {"menu"}})
disableCommand(headlessSpeech)
setCommandChecked(headlessSpeech, true)
setCommandPinned(headlessSpeech, true)
setCommandPulse(headlessFlash, true, "red", "#0000ff", 300)
)lua")));

        const QList<THeadlessSession::AddonCommand> commands = session.addonCommands();
        QCOMPARE(names(commands), (QStringList{qsl("Speech"), qsl("Flash"), qsl("Menu only")}));
        const THeadlessSession::AddonCommand& speech = commands.at(0);
        QCOMPARE(QString::number(speech.id), luaGlobalString(session.host(), "headlessSpeech"));
        QCOMPARE(speech.package, QString());
        QCOMPARE(speech.icon, qsl("/no/such/icon.png"));
        QCOMPARE(speech.tooltip, qsl("Say it"));
        QCOMPARE(speech.menuPath, qsl("Speech/Voices"));
        QCOMPARE(speech.shortcut, qsl("Ctrl+Alt+K"));
        QVERIFY(speech.onMenu && speech.onToolbar);
        QVERIFY(!speech.enabled);
        QVERIFY(speech.checkable && speech.checked);
        QVERIFY(speech.pinned);
        QVERIFY(!speech.pulse);

        const THeadlessSession::AddonCommand& flash = commands.at(1);
        QCOMPARE(QString::number(flash.id), luaGlobalString(session.host(), "headlessFlash"));
        QVERIFY(!flash.onMenu && flash.onToolbar);
        QVERIFY(flash.enabled && !flash.checkable && !flash.checked && !flash.pinned);
        QVERIFY(flash.pulse);
        QCOMPARE(flash.pulseColor1, QColor(Qt::red));
        QCOMPARE(flash.pulseColor2, QColor(Qt::blue));
        QCOMPARE(flash.pulseIntervalMs, 300);
        QVERIFY(commands.at(2).onMenu && !commands.at(2).onToolbar);

        QVERIFY(session.runLua(qsl("removeCommand(headlessFlash)")));
        QCOMPARE(names(session.addonCommands()), (QStringList{qsl("Speech"), qsl("Menu only")}));
        QVERIFY(session.shutdown(false));
        QVERIFY(session.addonCommands().isEmpty());
    }

    void test_aPackagesCommandNamesItsPackage()
    {
        const QString name = qsl("Headless Commands Package");
        QVERIFY(writeProfile(name));
        const QString packagePath = mConfigDir.filePath(qsl("commandsPackage.xml"));
        QVERIFY(writeFile(packagePath, packageXml(R"(addCommand({name = "Packaged"}))")));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY2(session.installPackage(packagePath) == THeadlessSession::PackageResult::installed, qPrintable(session.errorString()));
        const QList<THeadlessSession::AddonCommand> commands = session.addonCommands();
        QCOMPARE(names(commands), QStringList{qsl("Packaged")});
        QCOMPARE(commands.at(0).package, qsl("commandsPackage"));
        QVERIFY(session.shutdown(false));
    }

    void test_anotherProfilesCommandsAreNotListed()
    {
        const QString name = qsl("Headless Commands Own");
        const QString other = qsl("Headless Commands Other");
        QVERIFY(writeProfile(name));
        QVERIFY(writeProfile(other));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl("addCommand({name = 'Own'})")));
        QVERIFY(session.runLua(qsl("loadProfile([[%1]], true)").arg(other)));
        Host* pOther = HostManager::self()->getHost(other);
        QVERIFY(pOther);
        QVERIFY(pOther->getLuaInterpreter()->compileAndExecuteScript(qsl("addCommand({name = 'Theirs'})")));
        QCOMPARE(names(session.addonCommands()), QStringList{qsl("Own")});
        QVERIFY(session.shutdown(false));
    }
    void test_changesAreSentOncePerTurn()
    {
        const QString name = qsl("Headless Commands Changes");
        const QString other = qsl("Headless Commands Changes Other");
        QVERIFY(writeProfile(name));
        QVERIFY(writeProfile(other));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy changed(&session, &THeadlessSession::addonCommandsChanged);
        QVERIFY(session.runLua(qsl(R"lua(
headlessFirst = addCommand({name = "First"})
headlessSecond = addCommand({name = "Second"})
setCommandTooltip(headlessFirst, "Tip")
)lua")));
        QCOMPARE(changed.count(), 0);
        QVERIFY(changed.wait());
        QTest::qWait(50);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changed.at(0).at(0).toString(), name);

        changed.clear();
        QVERIFY(session.runLua(qsl(R"(setCommandTooltip(headlessFirst, "Tip"))")));
        QTest::qWait(50);
        QCOMPARE(changed.count(), 0);

        QVERIFY(session.runLua(qsl("loadProfile([[%1]], true)").arg(other)));
        Host* pOther = HostManager::self()->getHost(other);
        QVERIFY(pOther);
        QTest::qWait(50);
        changed.clear();
        QVERIFY(pOther->getLuaInterpreter()->compileAndExecuteScript(qsl("addCommand({name = 'Theirs'})")));
        QTest::qWait(50);
        QCOMPARE(changed.count(), 0);

        QVERIFY(session.runLua(qsl("removeCommand(headlessSecond)")));
        QVERIFY(changed.wait());
        QCOMPARE(changed.count(), 1);
        QVERIFY(session.shutdown(false));
    }

    void test_removingThePackageSendsAChange()
    {
        const QString name = qsl("Headless Commands Uninstall");
        QVERIFY(writeProfile(name));
        const QString packagePath = mConfigDir.filePath(qsl("commandsUninstalled.xml"));
        QVERIFY(writeFile(packagePath, packageXml(R"(addCommand({name = "Packaged"}))")));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy changed(&session, &THeadlessSession::addonCommandsChanged);
        QVERIFY2(session.installPackage(packagePath) == THeadlessSession::PackageResult::installed, qPrintable(session.errorString()));
        QTRY_VERIFY(changed.count() > 0);
        QCOMPARE(session.addonCommands().size(), 1);
        changed.clear();
        // Installing saves the profile, and a package is not removed while it is being saved
        QTRY_VERIFY(!session.host()->currentlySavingProfile());
        QVERIFY(session.runLua(qsl("uninstallPackage('commandsUninstalled')")));
        QVERIFY(session.addonCommands().isEmpty());
        QTRY_VERIFY(changed.count() > 0);
        QVERIFY(session.shutdown(false));
    }

    void test_resettingTheProfileSendsAChange()
    {
        const QString name = qsl("Headless Commands Reset");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy changed(&session, &THeadlessSession::addonCommandsChanged);
        QVERIFY(session.runLua(qsl("addCommand({name = 'Reset away'})")));
        QVERIFY(changed.wait());
        changed.clear();
        QVERIFY(session.runLua(qsl("resetProfile()")));
        QTRY_VERIFY(changed.count() > 0);
        QVERIFY(session.addonCommands().isEmpty());
        QVERIFY(session.shutdown(false));
    }
    void test_aClickRaisesSysCommandClicked()
    {
        const QString name = qsl("Headless Commands Click");
        const QString other = qsl("Headless Commands Click Other");
        QVERIFY(writeProfile(name));
        QVERIFY(writeProfile(other));
        THeadlessSession unstarted;
        QVERIFY(!unstarted.activateAddonCommand(1));
        QCOMPARE(unstarted.errorString(), qsl("No profile is open."));

        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
headlessClicks = ""
registerAnonymousEventHandler("sysCommandClicked", function(_, id) headlessClicks = headlessClicks .. id .. " " end)
headlessPlain = addCommand({name = "Plain"})
headlessToggle = addCommand({name = "Toggle"})
setCommandChecked(headlessToggle, false)
headlessOff = addCommand({name = "Off"})
disableCommand(headlessOff)
)lua")));
        const int plain = luaGlobalString(session.host(), "headlessPlain").toInt();
        const int toggle = luaGlobalString(session.host(), "headlessToggle").toInt();
        const int off = luaGlobalString(session.host(), "headlessOff").toInt();
        QSignalSpy changed(&session, &THeadlessSession::addonCommandsChanged);
        QTRY_VERIFY(changed.count() > 0);
        changed.clear();

        QVERIFY(session.activateAddonCommand(plain));
        QCOMPARE(luaGlobalString(session.host(), "headlessClicks"), qsl("%1 ").arg(plain));
        QVERIFY(!session.addonCommands().at(0).checked);
        QTest::qWait(50);
        QCOMPARE(changed.count(), 0);

        QVERIFY(session.activateAddonCommand(toggle));
        QVERIFY(session.addonCommands().at(1).checked);
        QVERIFY(changed.wait());
        QVERIFY(session.activateAddonCommand(toggle));
        QVERIFY(!session.addonCommands().at(1).checked);
        QCOMPARE(luaGlobalString(session.host(), "headlessClicks"), qsl("%1 %2 %2 ").arg(plain).arg(toggle));

        QVERIFY(!session.activateAddonCommand(off));
        QCOMPARE(session.errorString(), qsl("The command \"Off\" is disabled."));
        QVERIFY(!session.activateAddonCommand(off + 1000));
        QCOMPARE(session.errorString(), qsl("The profile has no command %1.").arg(off + 1000));
        QVERIFY(session.runLua(qsl("loadProfile([[%1]], true)").arg(other)));
        Host* pOther = HostManager::self()->getHost(other);
        QVERIFY(pOther);
        QVERIFY(pOther->getLuaInterpreter()->compileAndExecuteScript(qsl("headlessTheirs = addCommand({name = 'Theirs'})")));
        const int theirs = luaGlobalString(pOther, "headlessTheirs").toInt();
        QVERIFY(theirs > 0);
        QVERIFY(!session.activateAddonCommand(theirs));
        QCOMPARE(session.errorString(), qsl("The profile has no command %1.").arg(theirs));

        std::optional<bool> clickedMidWrite;
        connect(&session, &THeadlessSession::outputLine, this, [&session, &clickedMidWrite, plain](const QString&, const QString& text) {
            if (text == qsl("click now")) {
                clickedMidWrite = session.activateAddonCommand(plain);
            }
        });
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("click now\n"))lua")));
        QCOMPARE(clickedMidWrite, std::optional<bool>(false));
        QCOMPARE(luaGlobalString(session.host(), "headlessClicks"), qsl("%1 %2 %2 ").arg(plain).arg(toggle));
        QVERIFY(session.shutdown(false));
        QVERIFY(!session.activateAddonCommand(plain));
    }

    // As in the GUI, whose menu ticks the command before the event the stop holds is raised
    void test_theEmergencyStopHoldsTheClickButNotTheTick()
    {
        const QString name = qsl("Headless Commands Click Stopped");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
headlessClicks = ""
registerAnonymousEventHandler("sysCommandClicked", function(_, id) headlessClicks = headlessClicks .. id .. " " end)
headlessToggle = addCommand({name = "Toggle"})
setCommandChecked(headlessToggle, false)
)lua")));
        const int toggle = luaGlobalString(session.host(), "headlessToggle").toInt();
        QVERIFY(session.setScriptsStopped(true));
        QVERIFY(session.activateAddonCommand(toggle));
        QVERIFY(session.addonCommands().at(0).checked);
        QVERIFY(session.setScriptsStopped(false));
        QCOMPARE(luaGlobalString(session.host(), "headlessClicks"), QString());
        QVERIFY(session.activateAddonCommand(toggle));
        QCOMPARE(luaGlobalString(session.host(), "headlessClicks"), qsl("%1 ").arg(toggle));
        QVERIFY(session.shutdown(false));
    }
    // In Qt's order: the focused line may claim a key first, then the menu's shortcut takes it, then the line has it
    void test_aCommandsShortcutRunsAheadOfTheLine()
    {
        using KeyResult = THeadlessSession::KeyResult;
        const QString name = qsl("Headless Commands Shortcut");
        const QString other = qsl("Headless Commands Shortcut Other");
        QVERIFY(writeProfile(name));
        QVERIFY(writeProfile(other));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
headlessClicks = ""
headlessKeys = ""
registerAnonymousEventHandler("sysCommandClicked", function(_, id) headlessClicks = headlessClicks .. id .. " " end)
headlessShortcut = addCommand({name = "Shortcut", shortcut = "Ctrl+Alt+K"})
headlessPad = addCommand({name = "Pad", shortcut = "Ctrl+8"})
headlessEscape = addCommand({name = "Escape", shortcut = "Escape"})
headlessUp = addCommand({name = "Up", shortcut = "Up"})
headlessAll = addCommand({name = "All", shortcut = "Ctrl+A"})
headlessOff = addCommand({name = "Off", shortcut = "F5"})
disableCommand(headlessOff)
headlessTwo = addCommand({name = "Two", shortcut = "Ctrl+J, Ctrl+L"})
local m = mudlet.keymodifier
tempKey(m.Control + m.Alt, mudlet.key.K, [[headlessKeys = headlessKeys .. "Ctrl+Alt+K "]])
tempKey(m.Control + m.Keypad, mudlet.key["8"], [[headlessKeys = headlessKeys .. "Ctrl+Keypad 8 "]])
tempKey(m.Control, mudlet.key.A, [[headlessKeys = headlessKeys .. "Ctrl+A "]])
tempKey(m.None, mudlet.key.F5, [[headlessKeys = headlessKeys .. "F5 "]])
tempKey(m.Control, mudlet.key.J, [[headlessKeys = headlessKeys .. "Ctrl+J "]])
)lua")));
        const QString shortcut = luaGlobalString(session.host(), "headlessShortcut");
        const QString pad = luaGlobalString(session.host(), "headlessPad");
        const QString escape = luaGlobalString(session.host(), "headlessEscape");
        QCOMPARE(session.addonCommands().size(), 7);

        QCOMPARE(session.pressKey(Qt::Key_K, Qt::ControlModifier | Qt::AltModifier), KeyResult::commandRan);
        QCOMPARE(session.errorString(), qsl("That key is the shortcut of a command a script added to the menu, which ran."));
        QCOMPARE(session.pressKey(Qt::Key_8, Qt::ControlModifier | Qt::KeypadModifier), KeyResult::commandRan);
        QCOMPARE(session.pressKey(Qt::Key_Escape), KeyResult::commandRan);
        QCOMPARE(luaGlobalString(session.host(), "headlessClicks"), qsl("%1 %2 %3 ").arg(shortcut, pad, escape));
        QCOMPARE(luaGlobalString(session.host(), "headlessKeys"), QString());

        // The line claims these, a disabled command's shortcut does not fire, and one of two keys is not offered
#if defined(Q_OS_MACOS)
        // The arrows carry the keypad flag there, and the line moves through its history only on that
        QCOMPARE(session.pressKey(Qt::Key_Up, Qt::KeypadModifier), KeyResult::keyKept);
        QCOMPARE(session.pressKey(Qt::Key_Up), KeyResult::noBinding);
#else
        QCOMPARE(session.pressKey(Qt::Key_Up), KeyResult::keyKept);
        QCOMPARE(session.pressKey(Qt::Key_Up, Qt::KeypadModifier), KeyResult::noBinding);
#endif
        QCOMPARE(session.pressKey(Qt::Key_A, Qt::ControlModifier), KeyResult::bindingRan);
        QCOMPARE(session.pressKey(Qt::Key_F5), KeyResult::bindingRan);
        QCOMPARE(session.pressKey(Qt::Key_J, Qt::ControlModifier), KeyResult::bindingRan);
        QCOMPARE(luaGlobalString(session.host(), "headlessKeys"), qsl("Ctrl+A F5 Ctrl+J "));
        QCOMPARE(luaGlobalString(session.host(), "headlessClicks"), qsl("%1 %2 %3 ").arg(shortcut, pad, escape));

        // Another profile's command does not fire, even pinned, as addonCommands() and activateAddonCommand() leave
        // it out
        QVERIFY(session.runLua(qsl("loadProfile([[%1]], true)").arg(other)));
        Host* pOther = HostManager::self()->getHost(other);
        QVERIFY(pOther);
        QVERIFY(pOther->getLuaInterpreter()->compileAndExecuteScript(qsl(R"lua(
headlessClicks = ""
registerAnonymousEventHandler("sysCommandClicked", function(_, id) headlessClicks = headlessClicks .. id .. " " end)
headlessTheirs = addCommand({name = "Theirs", shortcut = "Ctrl+Alt+O"})
setCommandPinned(headlessTheirs, true)
)lua")));
        QCOMPARE(session.pressKey(Qt::Key_O, Qt::ControlModifier | Qt::AltModifier), KeyResult::noBinding);
        QVERIFY(!names(session.addonCommands()).contains(qsl("Theirs")));
        QVERIFY(!session.activateAddonCommand(luaGlobalString(pOther, "headlessTheirs").toInt()));
        QCOMPARE(luaGlobalString(pOther, "headlessClicks"), QString());
        QCOMPARE(luaGlobalString(session.host(), "headlessClicks"), qsl("%1 %2 %3 ").arg(shortcut, pad, escape));
        QVERIFY(session.shutdown(false));
    }

    // The command's script prints, and a handler of that deletes the session inside pressKey()
    void test_aSessionDeletedByItsShortcutsCommandIsLeftAlone()
    {
        const QString name = qsl("Headless Commands Shortcut Delete");
        QVERIFY(writeProfile(name));
        QObject owner;
        auto* pSession = new THeadlessSession(&owner);
        const QPointer<THeadlessSession> session(pSession);
        QCOMPARE(pSession->start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QPointer<Host> profile(pSession->host());
        QVERIFY(pSession->runLua(qsl(R"lua(
registerAnonymousEventHandler("sysCommandClicked", function() echo("clicked\n") end)
addCommand({name = "Delete", shortcut = "Ctrl+Alt+K"})
)lua")));
        // The allocator hands the session's memory straight back, where it can, and filling it makes a write into
        // the deleted session crash with no sanitizer to see it
        void* pReused = nullptr;
        connect(pSession, &THeadlessSession::localText, pSession, [pSession, &pReused]() {
            void* const pFreed = pSession;
            delete pSession;
            pReused = ::operator new(sizeof(THeadlessSession));
            if (pReused == pFreed) {
                std::memset(pReused, 0xa5, sizeof(THeadlessSession));
            }
        });
        const auto releaseReused = qScopeGuard([&pReused]() {
            ::operator delete(pReused);
        });
        QCOMPARE(pSession->pressKey(Qt::Key_K, Qt::ControlModifier | Qt::AltModifier), THeadlessSession::KeyResult::commandRan);
        QVERIFY2(session.isNull(), "SETUP: the handler did not delete the session");
        QTRY_VERIFY_WITH_TIMEOUT(profile.isNull(), 10000);
    }

    // The hidden-input box, a QLineEdit, claims fewer keys than the command line does
    void test_aCommandsShortcutRunsAheadOfHiddenInput()
    {
        using KeyResult = THeadlessSession::KeyResult;
        const QString name = qsl("Headless Commands Shortcut Hidden");
        QVERIFY(writeProfile(name));
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QVERIFY(MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first);
        QVERIFY(MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first);
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = true}), THeadlessSession::StartResult::started);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 10000);
        std::unique_ptr<QTcpSocket> game(server.nextPendingConnection());
        QVERIFY(session.runLua(qsl(R"lua(
headlessClicks = ""
registerAnonymousEventHandler("sysCommandClicked", function(_, id) headlessClicks = headlessClicks .. id .. " " end)
headlessUp = addCommand({name = "Up", shortcut = "Up"})
headlessLeft = addCommand({name = "Left", shortcut = "Ctrl+Left"})
)lua")));
        game->write("\xff\xfb\x01Password:\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(session.host()->passwordEntryWanted(), 10000);
        QCOMPARE(session.pressKey(Qt::Key_Up), KeyResult::commandRan);
        QCOMPARE(session.pressKey(Qt::Key_Left, Qt::ControlModifier), KeyResult::noBinding);
        QCOMPARE(luaGlobalString(session.host(), "headlessClicks"), qsl("%1 ").arg(luaGlobalString(session.host(), "headlessUp")));
        QVERIFY(session.shutdown(false));
    }
    void test_theButtonBarsScriptsMakeAreListed()
    {
        using Place = THeadlessSession::ButtonBar::Place;
        const QString name = qsl("Headless Buttons Temp");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QVERIFY(session.buttonBars().isEmpty());
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.buttonBars().isEmpty());
        QVERIFY(session.runLua(qsl(R"lua(
tempButtonToolbar("headlessTop", 0, 0)
tempButton("headlessTop", "first", 0)
tempButton("headlessTop", "second", 0)
setButtonStyleSheet("first", "color: red;")
tempButtonToolbar("headlessSide", 1, 1)
tempButton("headlessSide", "only", 0)
)lua")));
        const QList<THeadlessSession::ButtonBar> bars = session.buttonBars();
        QCOMPARE(barNames(bars), (QStringList{qsl("headlessTop"), qsl("headlessSide")}));
        QCOMPARE(bars.at(0).place, Place::top);
        QCOMPARE(bars.at(0).orientation, Qt::Horizontal);
        QCOMPARE(buttonNames(bars.at(0).buttons), (QStringList{qsl("first"), qsl("second")}));
        QCOMPARE(bars.at(0).buttons.at(0).styleSheet, qsl("color: red;"));
        QVERIFY(!bars.at(0).buttons.at(0).pushDown && !bars.at(0).buttons.at(0).menu);
        QCOMPARE(bars.at(1).place, Place::left);
        QCOMPARE(bars.at(1).orientation, Qt::Vertical);
        QCOMPARE(buttonNames(bars.at(1).buttons), QStringList{qsl("only")});
        QVERIFY(session.shutdown(false));
        QVERIFY(session.buttonBars().isEmpty());
    }

    // Built as the editor builds them, for what no Lua function makes: folders, push-down buttons, inactive items
    void test_theButtonBarsShowWhatTheGuiDraws()
    {
        using Place = THeadlessSession::ButtonBar::Place;
        const QString name = qsl("Headless Buttons Built");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        Host* host = session.host();

        TAction* bar = makeAction(host, nullptr, qsl("rightBar"), true);
        bar->mLocation = 3;
        bar->mOrientation = 1;
        bar->setButtonColumns(2);
        bar->setButtonFillerOffset(1);
        bar->css = qsl("background: blue;");
        TAction* push = makeAction(host, bar, qsl("push"));
        push->setIsPushDownButton(true);
        push->mButtonState = true;
        push->setIcon(qsl("/push.png"));
        TAction* folder = makeAction(host, bar, qsl("folder"), true);
        folder->setIcon(qsl("/folder.png"));
        makeAction(host, folder, qsl("inner"))->setIcon(qsl("/inner.png"));
        makeAction(host, folder, qsl("off"), false, false);
        TAction* sub = makeAction(host, folder, qsl("sub"), true);
        makeAction(host, sub, qsl("deep"));
        makeAction(host, bar, qsl("hidden"), false, false);

        TAction* floating = makeAction(host, nullptr, qsl("floatBar"), true);
        floating->mLocation = 4;
        TAction* floatingMenu = makeAction(host, floating, qsl("floatMenu"), true);
        makeAction(host, floatingMenu, qsl("a"));
        makeAction(host, floatingMenu, qsl("b"), false, false);

        makeAction(host, nullptr, qsl("offBar"), true, false);
        makeAction(host, nullptr, qsl("nowhereBar"), true)->mLocation = 1;

        TAction* package = makeAction(host, nullptr, qsl("package"), true);
        package->mPackageName = qsl("package");
        makeAction(host, package, qsl("packageBar"), true);
        makeAction(host, package, qsl("packageFloat"), true)->mLocation = 4;

        const QList<THeadlessSession::ButtonBar> bars = session.buttonBars();
        QCOMPARE(barNames(bars), (QStringList{qsl("rightBar"), qsl("floatBar"), qsl("packageBar")}));
        const THeadlessSession::ButtonBar& right = bars.at(0);
        QCOMPARE(right.id, bar->getID());
        QCOMPARE(right.place, Place::right);
        QCOMPARE(right.orientation, Qt::Vertical);
        QCOMPARE(right.columns, 2);
        QCOMPARE(right.fillerOffset, 1);
        QCOMPARE(right.styleSheet, qsl("background: blue;"));
        QCOMPARE(buttonNames(right.buttons), (QStringList{qsl("push"), qsl("folder")}));
        const THeadlessSession::Button& pushButton = right.buttons.at(0);
        QCOMPARE(pushButton.id, push->getID());
        QCOMPARE(pushButton.icon, qsl("/push.png"));
        QVERIFY(pushButton.pushDown && pushButton.checked && !pushButton.menu);
        const THeadlessSession::Button& folderButton = right.buttons.at(1);
        QVERIFY(folderButton.menu);
        QCOMPARE(buttonNames(folderButton.children), (QStringList{qsl("inner"), qsl("sub")}));
        // A button bar's menu shows each entry with the icon of the button it hangs from
        QCOMPARE(folderButton.children.at(0).icon, qsl("/folder.png"));
        QVERIFY(folderButton.children.at(1).menu);
        QCOMPARE(buttonNames(folderButton.children.at(1).children), QStringList{qsl("deep")});

        // A floating toolbar's menu holds an entry for the folder itself, then every child under it
        const THeadlessSession::ButtonBar& floatBar = bars.at(1);
        QCOMPARE(floatBar.place, Place::floating);
        QCOMPARE(buttonNames(floatBar.buttons), QStringList{qsl("floatMenu")});
        QCOMPARE(buttonNames(floatBar.buttons.at(0).children), QStringList{qsl("floatMenu")});
        QCOMPARE(buttonNames(floatBar.buttons.at(0).children.at(0).children), (QStringList{qsl("a"), qsl("b")}));

        QCOMPARE(bars.at(2).place, Place::top);

        QVERIFY(session.runLua(qsl("setButtonState('push', false)")));
        QVERIFY(!session.buttonBars().at(0).buttons.at(0).checked);
        QVERIFY(session.shutdown(false));
    }
    void test_buttonChangesAreSent()
    {
        const QString name = qsl("Headless Buttons Changes");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy changed(&session, &THeadlessSession::buttonsChanged);
        QVERIFY(session.runLua(qsl(R"lua(
tempButtonToolbar("headlessChanges", 0, 0)
tempButton("headlessChanges", "pushed", 0)
tempButton("headlessChanges", "plain", 0)
)lua")));
        QCOMPARE(changed.count(), 0);
        QVERIFY(changed.wait());
        QTest::qWait(50);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changed.at(0).at(0).toString(), name);

        // No Lua function makes a push-down button; the editor does
        session.host()->getActionUnit()->findAction(qsl("pushed"))->setIsPushDownButton(true);
        changed.clear();
        QVERIFY(session.runLua(qsl("setButtonState('pushed', true)")));
        QVERIFY(changed.wait());
        QVERIFY(session.buttonBars().at(0).buttons.at(0).checked);

        changed.clear();
        QVERIFY(session.runLua(qsl("setButtonState('pushed', true)")));
        QTest::qWait(50);
        QCOMPARE(changed.count(), 0);

        QVERIFY(session.runLua(qsl("setButtonStyleSheet('plain', 'color: green;')")));
        QVERIFY(changed.wait());
        QCOMPARE(session.buttonBars().at(0).buttons.at(1).styleSheet, qsl("color: green;"));
        QVERIFY(session.shutdown(false));
    }

    void test_installingAPackageWithAToolbarSendsAChange()
    {
        const QString name = qsl("Headless Buttons Package");
        QVERIFY(writeProfile(name));
        const QString packagePath = mConfigDir.filePath(qsl("buttonsPackaged.xml"));
        QVERIFY(writeFile(packagePath, R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<ActionPackage><ActionGroup isActive="yes" isFolder="yes" isPushButton="no" isFlatButton="no" useCustomLayout="no">
<name>packagedBar</name><packageName></packageName><script></script><css></css><location>0</location>
<Action isActive="yes" isFolder="no" isPushButton="no" isFlatButton="no" useCustomLayout="no">
<name>packagedButton</name><packageName></packageName><script></script><css></css></Action>
</ActionGroup></ActionPackage>
</MudletPackage>
)"));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QSignalSpy changed(&session, &THeadlessSession::buttonsChanged);
        QVERIFY2(session.installPackage(packagePath) == THeadlessSession::PackageResult::installed, qPrintable(session.errorString()));
        QTRY_VERIFY(changed.count() > 0);
        const QList<THeadlessSession::ButtonBar> bars = session.buttonBars();
        QCOMPARE(barNames(bars), QStringList{qsl("packagedBar")});
        QCOMPARE(buttonNames(bars.at(0).buttons), QStringList{qsl("packagedButton")});
        QVERIFY(session.shutdown(false));
    }
    void test_pressingAButtonRunsItAsTheGuiDoes()
    {
        const QString name = qsl("Headless Buttons Press");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QVERIFY(!session.pressButton(1));
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
headlessSent = ""
tempAlias("^(look|sit|stand)$", [[headlessSent = headlessSent .. matches[2] .. "/" .. getButtonState() .. " "]])
)lua")));
        Host* host = session.host();
        TAction* bar = makeAction(host, nullptr, qsl("pressBar"), true);
        TAction* look = makeAction(host, bar, qsl("look"));
        look->setCommandButtonDown(qsl("look"));
        TAction* posture = makeAction(host, bar, qsl("posture"));
        posture->setIsPushDownButton(true);
        posture->setCommandButtonDown(qsl("sit"));
        posture->setCommandButtonUp(qsl("stand"));
        TAction* menu = makeAction(host, bar, qsl("menu"), true);
        QVERIFY(menu->setScript(qsl("headlessMenuOpened = 'yes'")));
        TAction* entry = makeAction(host, menu, qsl("entry"));
        QVERIFY(entry->setScript(qsl("headlessEntryRan = 'yes'")));
        TAction* off = makeAction(host, bar, qsl("off"), false, false);
        QSignalSpy changed(&session, &THeadlessSession::buttonsChanged);

        QVERIFY2(session.pressButton(look->getID()), qPrintable(session.errorString()));
        QCOMPARE(luaGlobalString(host, "headlessSent"), qsl("look/1 "));
        QVERIFY(session.pressButton(posture->getID()));
        QVERIFY(session.buttonBars().at(0).buttons.at(1).checked);
        QVERIFY(changed.wait());
        QVERIFY(session.pressButton(posture->getID()));
        QCOMPARE(luaGlobalString(host, "headlessSent"), qsl("look/1 sit/2 stand/1 "));
        QVERIFY(!session.buttonBars().at(0).buttons.at(1).checked);

        QVERIFY(session.pressButton(menu->getID()));
        QCOMPARE(luaGlobalString(host, "headlessMenuOpened"), qsl("yes"));
        QVERIFY(session.pressButton(entry->getID()));
        QCOMPARE(luaGlobalString(host, "headlessEntryRan"), qsl("yes"));

        QVERIFY(!session.pressButton(off->getID()));
        QCOMPARE(session.errorString(), qsl("The profile shows no button %1.").arg(off->getID()));
        QVERIFY(!session.pressButton(bar->getID()));
        QVERIFY(!session.pressButton(999999));
        QVERIFY(session.shutdown(false));
    }

    void test_pressingAButtonIsRefusedFromAHandler()
    {
        const QString name = qsl("Headless Buttons Press Refused");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
tempButtonToolbar("refusedBar", 0, 0)
tempButton("refusedBar", "refused", 0)
)lua")));
        const int id = session.buttonBars().at(0).buttons.at(0).id;
        std::optional<bool> pressedMidWrite;
        connect(&session, &THeadlessSession::outputLine, this, [&session, &pressedMidWrite, id](const QString&, const QString& text) {
            if (text == qsl("press now")) {
                pressedMidWrite = session.pressButton(id);
            }
        });
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("press now\n"))lua")));
        QCOMPARE(pressedMidWrite, std::optional<bool>(false));
        QVERIFY(session.errorString().startsWith(qsl("Refused from a handler of outputLine")));
        QVERIFY(session.pressButton(id));
        QVERIFY(session.shutdown(false));
    }
};

#include "HeadlessSessionCommandsTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionCommandsTest)
