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
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <optional>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
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
};

#include "HeadlessSessionCommandsTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionCommandsTest)
