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
};

#include "HeadlessSessionCommandsTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionCommandsTest)
