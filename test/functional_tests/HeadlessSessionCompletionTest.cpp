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
 * What the GUI's command line completes as the player types, as THeadlessSession gives it to an app that
 * embeds Mudlet. Each test makes a session of its own, and with it the process's one profile pool.
 *
 * Run with: ctest -R HeadlessSessionCompletionTest -V
 */

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "THeadlessSession.h"

#include "GroupedTest.h"

class HeadlessSessionCompletionTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    static bool writeProfile(const QString& name)
    {
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        if (!QDir().mkpath(saveFolder)) {
            return false;
        }
        QFile saveFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder));
        if (!saveFile.open(QIODevice::WriteOnly)) {
            return false;
        }
        saveFile.write(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<HostPackage><Host></Host></HostPackage>
</MudletPackage>
)");
        return true;
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

    void test_theSuggestionsScriptsAddAreRead()
    {
        const QString name = qsl("Headless Suggestions");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QVERIFY(session.commandLineSuggestions().isEmpty());
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.commandLineSuggestions().isEmpty());

        QVERIFY(session.runLua(qsl(R"lua(
addCmdLineSuggestion("fireball")
addCmdLineSuggestion("main", "heal")
addCmdLineSuggestion("fireball")
)lua")));
        QCOMPARE(session.commandLineSuggestions(), QStringList({qsl("fireball"), qsl("heal")}));
        QVERIFY(session.runLua(qsl(R"lua(
removeCmdLineSuggestion("heal")
removeCmdLineSuggestion("never added")
)lua")));
        QCOMPARE(session.commandLineSuggestions(), QStringList({qsl("fireball")}));
        QVERIFY(session.runLua(qsl(R"lua(clearCmdLineSuggestions())lua")));
        QVERIFY(session.commandLineSuggestions().isEmpty());
        QVERIFY(session.commandLineBlacklist().isEmpty());
        QVERIFY(session.shutdown(false));
        QVERIFY(session.commandLineSuggestions().isEmpty());
    }

    void test_theBlacklistScriptsAddIsRead()
    {
        const QString name = qsl("Headless Blacklist");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
addCmdLineBlacklist("the")
addCmdLineBlacklist("and")
addCmdLineBlacklist("the")
)lua")));
        QCOMPARE(session.commandLineBlacklist(), QStringList({qsl("and"), qsl("the")}));
        QVERIFY(session.commandLineSuggestions().isEmpty());
        QVERIFY(session.runLua(qsl(R"lua(removeCmdLineBlacklist("and"))lua")));
        QCOMPARE(session.commandLineBlacklist(), QStringList({qsl("the")}));
        QVERIFY(session.runLua(qsl(R"lua(clearCmdLineBlacklist("main"))lua")));
        QVERIFY(session.commandLineBlacklist().isEmpty());
        QVERIFY(session.shutdown(false));
    }

    // Each command line has its own words, as each TCommandLine does
    void test_anotherCommandLinesWordsAreNotTheMainLines()
    {
        const QString name = qsl("Headless Other Line Suggestions");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        // Nothing back is the line found
        QVERIFY2(session.runLua(qsl(R"lua(
createCommandLine("headlessLine", 0, 0, 200, 20)
createMiniConsole("headlessConsole", 0, 30, 200, 100)
enableCommandLine("headlessConsole")
assert(select("#", addCmdLineSuggestion("headlessLine", "spell")) == 0)
assert(select("#", addCmdLineBlacklist("headlessLine", "word")) == 0)
assert(select("#", addCmdLineSuggestion("headlessConsole", "potion")) == 0)
assert(select("#", addCmdLineBlacklist("headlessConsole", "other")) == 0)
)lua")),
                 qPrintable(session.errorString()));
        QVERIFY(session.commandLineSuggestions().isEmpty());
        QVERIFY(session.commandLineBlacklist().isEmpty());
        QVERIFY2(session.runLua(qsl(R"lua(
assert(select(2, addCmdLineSuggestion("headlessNoSuchLine", "word")) == 'command line "headlessNoSuchLine" not found')
addCmdLineSuggestion("mainword")
clearCmdLineSuggestions("headlessLine")
clearCmdLineSuggestions("headlessConsole")
)lua")),
                 qPrintable(session.errorString()));
        QCOMPARE(session.commandLineSuggestions(), QStringList({qsl("mainword")}));
        QVERIFY(session.shutdown(false));
    }
};

#include "HeadlessSessionCompletionTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionCompletionTest)
