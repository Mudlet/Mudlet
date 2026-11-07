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
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <memory>
#include <optional>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "THeadlessSession.h"
#include "TLuaInterpreter.h"

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

    static bool listenAsTheGame(QTcpServer& server, const QString& name)
    {
        return server.listen(QHostAddress::LocalHost) && MudletApp::writeProfileData(name, qsl("url"), qsl("127.0.0.1")).first
               && MudletApp::writeProfileData(name, qsl("port"), QString::number(server.serverPort())).first;
    }

    static QStringList texts(const QSignalSpy& spy)
    {
        QStringList result;
        for (const auto& arguments : spy) {
            result << arguments.at(1).toString();
        }
        return result;
    }

    // What a script's getCmdLine() reads
    static QString scriptsLine(THeadlessSession& session)
    {
        if (!session.runLua(qsl("headlessCmdLine = getCmdLine()"))) {
            return qsl("<getCmdLine() failed>");
        }
        lua_State* L = session.host()->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, "headlessCmdLine");
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
    }

    // Started against a game that has sent the line
    static bool startWithGameLine(THeadlessSession& session, const QString& name, QTcpServer& server, std::unique_ptr<QTcpSocket>& game, const QByteArray& line)
    {
        if (!writeProfile(name) || !listenAsTheGame(server, name)) {
            return false;
        }
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        if (session.start({.profile = name, .connect = true}) != THeadlessSession::StartResult::started) {
            return false;
        }
        // The connection is made on this thread's event loop, which waitForNewConnection() would block
        for (int waited = 0; !server.hasPendingConnections() && waited < 10000; waited += 50) {
            QTest::qWait(50);
        }
        if (!server.hasPendingConnections()) {
            return false;
        }
        game.reset(server.nextPendingConnection());
        game->write(line + "\r\n");
        const QString text = QString::fromUtf8(line);
        for (int waited = 0; !texts(output).contains(text) && waited < 10000; waited += 50) {
            QTest::qWait(50);
        }
        return texts(output).contains(text);
    }

    // Started against a game, with the lines sent, the newest last
    static bool startWithHistory(THeadlessSession& session, const QString& name, QTcpServer& server, std::unique_ptr<QTcpSocket>& game, const QStringList& lines)
    {
        if (!startWithGameLine(session, name, server, game, "welcome")) {
            return false;
        }
        for (const QString& line : lines) {
            if (!session.submit(line)) {
                return false;
            }
        }
        QStringList newestFirst(lines.crbegin(), lines.crend());
        return session.commandHistory() == newestFirst;
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

    void test_tabCompletesAWordTheGameSent()
    {
        THeadlessSession session;
        QCOMPARE(session.completeCommandLine(), std::nullopt);
        QCOMPARE(session.errorString(), qsl("No profile is open."));
        QTcpServer server;
        std::unique_ptr<QTcpSocket> game;
        QVERIFY(startWithGameLine(session, qsl("Headless Tab Game Word"), server, game, "a goblin attacks"));

        QVERIFY(session.setCommandLineText(qsl("kill gob")));
        QCOMPARE(session.completeCommandLine(), std::optional<QString>(qsl("kill goblin")));
        QCOMPARE(scriptsLine(session), qsl("kill goblin"));
        QVERIFY(session.setCommandLineText(qsl("kill orc")));
        QCOMPARE(session.completeCommandLine(), std::nullopt);
        QCOMPARE(session.errorString(), qsl("No word completes what is typed."));
        QCOMPARE(scriptsLine(session), qsl("kill orc"));
        QVERIFY(session.shutdown(false));
    }

    // The most recently seen first, as the GUI offers them, stopping at either end
    void test_tabGoesThroughTheWordsInTheGuisOrder()
    {
        THeadlessSession session;
        QTcpServer server;
        std::unique_ptr<QTcpSocket> game;
        QVERIFY(startWithGameLine(session, qsl("Headless Tab Order"), server, game, "a goblet and a goblin"));

        QVERIFY(session.setCommandLineText(qsl("gob")));
        QCOMPARE(session.completeCommandLine(), std::optional<QString>(qsl("goblin")));
        QCOMPARE(session.completeCommandLine(), std::optional<QString>(qsl("goblet")));
        QCOMPARE(session.completeCommandLine(), std::optional<QString>(qsl("goblet")));
        QCOMPARE(session.completeCommandLine(false), std::optional<QString>(qsl("goblin")));
        QCOMPARE(session.completeCommandLine(false), std::optional<QString>(qsl("goblin")));
        QCOMPARE(session.completeCommandLine(true), std::optional<QString>(qsl("goblet")));
        QVERIFY(session.shutdown(false));
    }

    void test_tabCompletesASuggestionButNoBlacklistedWord()
    {
        const QString name = qsl("Headless Tab Suggestion");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.runLua(qsl(R"lua(
addCmdLineSuggestion("fireball")
addCmdLineSuggestion("gobstopper")
addCmdLineSuggestion("goblin")
addCmdLineBlacklist("GOBLIN")
)lua")));
        QVERIFY(session.setCommandLineText(qsl("cast fir")));
        QCOMPARE(session.completeCommandLine(), std::optional<QString>(qsl("cast fireball")));
        QVERIFY(session.setCommandLineText(qsl("eat gob")));
        QCOMPARE(session.completeCommandLine(), std::optional<QString>(qsl("eat gobstopper")));
        QCOMPARE(session.completeCommandLine(), std::optional<QString>(qsl("eat gobstopper")));
        QVERIFY(session.shutdown(false));
    }

    // As a key on the GUI's line: one that changes it starts a new cycle, Escape ends the cycle, and the line the
    // completion set handed back as typed changes nothing
    void test_editingTheLineStartsANewCycle()
    {
        THeadlessSession session;
        QTcpServer server;
        std::unique_ptr<QTcpSocket> game;
        QVERIFY(startWithGameLine(session, qsl("Headless Tab Reset"), server, game, "a goblet and a goblin"));

        QVERIFY(session.setCommandLineText(qsl("gob")));
        QCOMPARE(session.completeCommandLine(), std::optional<QString>(qsl("goblin")));
        QVERIFY(session.setCommandLineText(qsl("goblin")));
        QCOMPARE(session.completeCommandLine(), std::optional<QString>(qsl("goblet")));
        QVERIFY(session.setCommandLineText(qsl("go")));
        QCOMPARE(session.completeCommandLine(), std::optional<QString>(qsl("goblin")));

        QCOMPARE(session.completeCommandLine(), std::optional<QString>(qsl("goblet")));
        QCOMPARE(session.pressKey(Qt::Key_Escape), THeadlessSession::KeyResult::keyKept);
        // Read again whole, and no word goes on from "goblet"
        QCOMPARE(session.completeCommandLine(), std::nullopt);
        QVERIFY(session.shutdown(false));
    }

    // As Backspace on the GUI's line, as Mudlet cannot tell it from another key
    void test_anEditBackToTheCompletedLineEndsTheCycle()
    {
        THeadlessSession session;
        QTcpServer server;
        std::unique_ptr<QTcpSocket> game;
        QVERIFY(startWithGameLine(session, qsl("Headless Tab Edit Back"), server, game, "a goblet and a goblin"));

        QVERIFY(session.setCommandLineText(qsl("gob")));
        QCOMPARE(session.completeCommandLine(), std::optional<QString>(qsl("goblin")));
        QVERIFY(session.setCommandLineText(qsl("goblinx")));
        QVERIFY(session.setCommandLineText(qsl("goblin")));
        // No longer word starts with "goblin"
        QCOMPARE(session.completeCommandLine(), std::nullopt);
        QCOMPARE(scriptsLine(session), qsl("goblin"));
        QVERIFY(session.shutdown(false));
    }

    // A recall or a script's printCmdLine() between two presses of Tab: the new line is completed, never overwritten
    void test_aLineSetBetweenTabsIsCompleted()
    {
        THeadlessSession session;
        QTcpServer server;
        std::unique_ptr<QTcpSocket> game;
        QVERIFY(startWithGameLine(session, qsl("Headless Tab Line Set"), server, game, "a goblet and a goblin"));

        QVERIFY(session.setCommandLineText(qsl("gob")));
        QCOMPARE(session.completeCommandLine(), std::optional<QString>(qsl("goblin")));
        QVERIFY(session.setCommandLineText(qsl("kill gob"), THeadlessSession::CommandLineEdit::notTyped));
        QCOMPARE(session.completeCommandLine(), std::optional<QString>(qsl("kill goblin")));
        QCOMPARE(session.completeCommandLine(), std::optional<QString>(qsl("kill goblet")));
        QVERIFY(session.runLua(qsl(R"lua(printCmdLine("eat gob"))lua")));
        QCOMPARE(session.completeCommandLine(), std::optional<QString>(qsl("eat goblin")));
        QVERIFY(session.shutdown(false));
    }

    // As the GUI's Tab once it has sent a line, not its very first, which puts a word of the buffer there
    void test_tabCompletesNothingOnAnEmptyLine()
    {
        THeadlessSession session;
        QTcpServer server;
        std::unique_ptr<QTcpSocket> game;
        QVERIFY(startWithGameLine(session, qsl("Headless Tab Empty"), server, game, "a goblet and a goblin"));

        QCOMPARE(session.completeCommandLine(), std::nullopt);
        QCOMPARE(session.errorString(), qsl("No word completes what is typed."));
        QCOMPARE(scriptsLine(session), QString());
        QVERIFY(session.shutdown(false));
    }

    // As the GUI completes no password
    void test_nothingIsCompletedWhileTheGameHidesWhatIsTyped()
    {
        THeadlessSession session;
        QTcpServer server;
        std::unique_ptr<QTcpSocket> game;
        QVERIFY(startWithGameLine(session, qsl("Headless Tab Hidden"), server, game, "a goblin"));
        QSignalSpy output(&session, &THeadlessSession::outputLine);

        QVERIFY(session.setCommandLineText(qsl("gob")));
        game->write("\xff\xfb\x01Password:\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(texts(output).contains(qsl("Password:")), 10000);
        QCOMPARE(session.completeCommandLine(), std::nullopt);
        QCOMPARE(session.errorString(), qsl("Nothing is completed while the game hides what is typed."));
        QCOMPARE(session.completeCommandLineFromHistory(), std::nullopt);
        QCOMPARE(session.errorString(), qsl("Nothing is completed while the game hides what is typed."));
        game->write("\xff\xfc\x01Welcome\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(texts(output).contains(qsl("Welcome")), 10000);
        QVERIFY(session.setCommandLineText(qsl("gob")));
        QCOMPARE(session.completeCommandLine(), std::optional<QString>(qsl("goblin")));
        QVERIFY(session.shutdown(false));
    }

    void test_tabIsRefusedMidWrite()
    {
        const QString name = qsl("Headless Tab Mid Write");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setCommandLineText(qsl("gob")));
        std::optional<std::optional<QString>> midWrite;
        QString midWriteError;
        connect(&session, &THeadlessSession::outputLine, this, [&session, &midWrite, &midWriteError](const QString&, const QString& text) {
            if (text == qsl("a goblin")) {
                midWrite = session.completeCommandLine();
                midWriteError = session.errorString();
            }
        });
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("a goblin\n"))lua")));
        QCOMPARE(midWrite, std::optional<std::optional<QString>>(std::optional<QString>()));
        QVERIFY2(midWriteError.startsWith(qsl("Refused from a handler")), qPrintable(midWriteError));
        QCOMPARE(scriptsLine(session), qsl("gob"));
        QCOMPARE(session.completeCommandLine(), std::optional<QString>(qsl("goblin")));
        QVERIFY(session.shutdown(false));
    }

    // As Up with "Highlight history" on: the newest line sent that starts with what was typed, then older ones, then
    // what was typed again
    void test_upCompletesALineSentBefore()
    {
        THeadlessSession unstarted;
        QCOMPARE(unstarted.completeCommandLineFromHistory(), std::nullopt);
        QCOMPARE(unstarted.errorString(), qsl("No profile is open."));
        THeadlessSession session;
        QTcpServer server;
        std::unique_ptr<QTcpSocket> game;
        QVERIFY(startWithGameLine(session, qsl("Headless Up Walk"), server, game, "welcome"));
        for (const QString& line : {qsl("eat"), qsl("look north"), qsl("look south")}) {
            QVERIFY(session.submit(line));
        }
        QCOMPARE(session.commandHistory(), (QStringList{qsl("look south"), qsl("look north"), qsl("eat")}));

        QVERIFY(session.setCommandLineText(qsl("look")));
        QCOMPARE(session.completeCommandLineFromHistory(), std::optional<QString>(qsl("look south")));
        QCOMPARE(scriptsLine(session), qsl("look south"));
        QCOMPARE(session.completeCommandLineFromHistory(), std::optional<QString>(qsl("look north")));
        QCOMPARE(session.completeCommandLineFromHistory(), std::optional<QString>(qsl("look")));
        QCOMPARE(scriptsLine(session), qsl("look"));
        QCOMPARE(session.completeCommandLineFromHistory(), std::optional<QString>(qsl("look south")));

        QVERIFY(session.setCommandLineText(qsl("ea")));
        QCOMPARE(session.completeCommandLineFromHistory(), std::optional<QString>(qsl("eat")));
        QCOMPARE(session.completeCommandLineFromHistory(), std::optional<QString>(qsl("eat")));

        QVERIFY(session.setCommandLineText(qsl("xyz")));
        QCOMPARE(session.completeCommandLineFromHistory(), std::nullopt);
        QCOMPARE(session.errorString(), qsl("No line sent before starts with what is typed."));
        QCOMPARE(scriptsLine(session), qsl("xyz"));
        QVERIFY(session.setCommandLineText(QString()));
        QCOMPARE(session.completeCommandLineFromHistory(), std::nullopt);
        QCOMPARE(session.errorString(), qsl("Nothing is typed to complete; commandHistory() holds the lines to recall."));
        QVERIFY(session.shutdown(false));
    }

    void test_editingTheLineStartsTheHistoryWalkAgain()
    {
        THeadlessSession session;
        QTcpServer server;
        std::unique_ptr<QTcpSocket> game;
        QVERIFY(startWithGameLine(session, qsl("Headless Up Reset"), server, game, "welcome"));
        for (const QString& line : {qsl("eat"), qsl("look north"), qsl("look south")}) {
            QVERIFY(session.submit(line));
        }

        QVERIFY(session.setCommandLineText(qsl("look")));
        QCOMPARE(session.completeCommandLineFromHistory(), std::optional<QString>(qsl("look south")));
        QCOMPARE(session.completeCommandLineFromHistory(), std::optional<QString>(qsl("look north")));
        // The line it left, handed back, is no edit
        QVERIFY(session.setCommandLineText(qsl("look north")));
        QCOMPARE(session.completeCommandLineFromHistory(), std::optional<QString>(qsl("look")));

        QVERIFY(session.setCommandLineText(qsl("look n")));
        QCOMPARE(session.completeCommandLineFromHistory(), std::optional<QString>(qsl("look north")));
        QCOMPARE(session.completeCommandLineFromHistory(), std::optional<QString>(qsl("look n")));

        QVERIFY(session.shutdown(false));
    }

    // As Backspace on the GUI's line forgets the line the walk found, so that line is no longer cut back
    void test_anEditBackToTheCompletedLineStartsTheWalkAgain()
    {
        THeadlessSession session;
        QTcpServer server;
        std::unique_ptr<QTcpSocket> game;
        QVERIFY(startWithHistory(session, qsl("Headless Up Edit Back"), server, game, {qsl("eat"), qsl("look north"), qsl("look south")}));

        QVERIFY(session.setCommandLineText(qsl("look")));
        QCOMPARE(session.completeCommandLineFromHistory(), std::optional<QString>(qsl("look south")));
        QVERIFY(session.setCommandLineText(qsl("look south x")));
        QVERIFY(session.setCommandLineText(qsl("look south")));
        QCOMPARE(session.completeCommandLineFromHistory(), std::optional<QString>(qsl("look south")));
        // No older line starts with "look south"
        QCOMPARE(session.completeCommandLineFromHistory(), std::optional<QString>(qsl("look south")));
        QVERIFY(session.shutdown(false));
    }

    // The line Up leaves is a new line to Tab, as one the app or a script sets
    void test_tabAfterUpCompletesTheLineUpLeft()
    {
        THeadlessSession session;
        QTcpServer server;
        std::unique_ptr<QTcpSocket> game;
        QVERIFY(startWithGameLine(session, qsl("Headless Tab After Up"), server, game, "a goblet and a goblin"));
        QVERIFY(session.submit(qsl("kill goblin with a gob")));

        QVERIFY(session.setCommandLineText(qsl("kill gob")));
        QCOMPARE(session.completeCommandLine(), std::optional<QString>(qsl("kill goblin")));
        QCOMPARE(session.completeCommandLineFromHistory(), std::optional<QString>(qsl("kill goblin with a gob")));
        QCOMPARE(session.completeCommandLine(), std::optional<QString>(qsl("kill goblin with a goblin")));
        QCOMPARE(scriptsLine(session), qsl("kill goblin with a goblin"));
        QVERIFY(session.shutdown(false));
    }

    // Escape and a script's selectCmdLineText() select the GUI's whole line, so its Up recalls whole lines, "eat"
    // among them, until the line is typed into; a script's printCmdLine() leaves none of it selected
    void test_upAfterEscapeRecallsWholeLines()
    {
        THeadlessSession session;
        QTcpServer server;
        std::unique_ptr<QTcpSocket> game;
        QVERIFY(startWithHistory(session, qsl("Headless Up Escape"), server, game, {qsl("look north"), qsl("eat"), qsl("look south")}));
        const QString wholeLines = qsl("The whole line is selected, so the up arrow recalls whole lines; commandHistory() holds them.");

        QVERIFY(session.setCommandLineText(qsl("look")));
        QCOMPARE(session.completeCommandLineFromHistory(), std::optional<QString>(qsl("look south")));
        QCOMPARE(session.pressKey(Qt::Key_Escape), THeadlessSession::KeyResult::keyKept);
        QCOMPARE(session.completeCommandLineFromHistory(), std::nullopt);
        QCOMPARE(session.errorString(), wholeLines);
        QCOMPARE(scriptsLine(session), qsl("look south"));

        QVERIFY(session.setCommandLineText(qsl("look")));
        QCOMPARE(session.completeCommandLineFromHistory(), std::optional<QString>(qsl("look south")));
        QCOMPARE(session.completeCommandLineFromHistory(), std::optional<QString>(qsl("look north")));

        QVERIFY(session.setCommandLineText(qsl("look n")));
        QVERIFY(session.runLua(qsl("selectCmdLineText()")));
        QCOMPARE(session.completeCommandLineFromHistory(), std::nullopt);
        QCOMPARE(session.errorString(), wholeLines);
        QVERIFY(session.runLua(qsl(R"lua(printCmdLine("look"))lua")));
        QCOMPARE(session.completeCommandLineFromHistory(), std::optional<QString>(qsl("look south")));
        QVERIFY(session.shutdown(false));
    }

    // As the GUI selects the whole of a line it recalls, which the app's own recall stands for
    void test_upOnALineSetAsNotTypedRecallsWholeLines()
    {
        THeadlessSession session;
        QTcpServer server;
        std::unique_ptr<QTcpSocket> game;
        QVERIFY(startWithHistory(session, qsl("Headless Up Not Typed"), server, game, {qsl("look north"), qsl("eat"), qsl("look south")}));

        QVERIFY(session.setCommandLineText(qsl("look"), THeadlessSession::CommandLineEdit::notTyped));
        QCOMPARE(session.completeCommandLineFromHistory(), std::nullopt);
        QCOMPARE(session.errorString(), qsl("The whole line is selected, so the up arrow recalls whole lines; commandHistory() holds them."));
        QVERIFY(session.setCommandLineText(qsl("look n")));
        QCOMPARE(session.completeCommandLineFromHistory(), std::optional<QString>(qsl("look north")));
        QVERIFY(session.shutdown(false));
    }

    // As the GUI selects a line it keeps after a send
    void test_upAfterAKeptSubmitRecallsWholeLines()
    {
        THeadlessSession session;
        QTcpServer server;
        std::unique_ptr<QTcpSocket> game;
        QVERIFY(startWithHistory(session, qsl("Headless Up Kept"), server, game, {qsl("look north"), qsl("eat")}));
        session.host()->mAutoClearCommandLineAfterSend = false;

        QVERIFY(session.submit(qsl("look south")));
        QCOMPARE(scriptsLine(session), qsl("look south"));
        QCOMPARE(session.completeCommandLineFromHistory(), std::nullopt);
        QCOMPARE(session.errorString(), qsl("The whole line is selected, so the up arrow recalls whole lines; commandHistory() holds them."));
        QVERIFY(session.setCommandLineText(qsl("look")));
        QCOMPARE(session.completeCommandLineFromHistory(), std::optional<QString>(qsl("look south")));
        QVERIFY(session.shutdown(false));
    }

    void test_upWithHighlightHistoryOffRecallsWholeLines()
    {
        THeadlessSession session;
        QTcpServer server;
        std::unique_ptr<QTcpSocket> game;
        QVERIFY(startWithHistory(session, qsl("Headless Up No Highlight"), server, game, {qsl("look north"), qsl("eat")}));
        session.host()->mHighlightHistory = false;

        QVERIFY(session.setCommandLineText(qsl("look")));
        QCOMPARE(session.completeCommandLineFromHistory(), std::nullopt);
        QCOMPARE(session.errorString(), qsl("\"Highlight history\" is off, so the up arrow recalls whole lines; commandHistory() holds them."));
        QCOMPARE(scriptsLine(session), qsl("look"));
        QVERIFY(session.shutdown(false));
    }

    void test_upIsRefusedMidWrite()
    {
        const QString name = qsl("Headless Up Mid Write");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(session.setCommandLineText(qsl("look")));
        std::optional<std::optional<QString>> midWrite;
        QString midWriteError;
        connect(&session, &THeadlessSession::outputLine, this, [&session, &midWrite, &midWriteError](const QString&, const QString& text) {
            if (text == qsl("you see")) {
                midWrite = session.completeCommandLineFromHistory();
                midWriteError = session.errorString();
            }
        });
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(qsl("refused")));
        QVERIFY(session.runLua(qsl(R"lua(feedTriggers("you see\n"))lua")));
        QCOMPARE(midWrite, std::optional<std::optional<QString>>(std::optional<QString>()));
        QVERIFY2(midWriteError.startsWith(qsl("Refused from a handler")), qPrintable(midWriteError));
        QCOMPARE(scriptsLine(session), qsl("look"));
        QVERIFY(session.shutdown(false));
    }

    // As the GUI flashes its taskbar entry for game text while it is minimized, and only then, the flash lasting until
    // the player comes back
    void test_newGameTextWhileMinimizedAsksForAttention()
    {
        const QString name = qsl("Headless New Data Alert");
        THeadlessSession session;
        QTcpServer server;
        std::unique_ptr<QTcpSocket> game;
        QVERIFY(startWithGameLine(session, name, server, game, "welcome"));
        QSignalSpy output(&session, &THeadlessSession::outputLine);
        QSignalSpy attention(&session, &THeadlessSession::attentionRequested);
        // The bare profile this test writes has no say, which loads as off
        session.host()->mAlertOnNewData = true;

        game->write("in front\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(texts(output).contains(qsl("in front")), 10000);
        QVERIFY(session.setApplicationActive(false));
        game->write("behind another window\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(texts(output).contains(qsl("behind another window")), 10000);
        QTest::qWait(100);
        QVERIFY(attention.isEmpty());

        // The bare carriage return the posting timer then finishes the line with is no new text
        session.host()->mTelnet.setPostingTimeout(1000);
        game->write("almost\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(texts(output).contains(qsl("almost")), 10000);
        session.setWindowMinimized(true);
        QVERIFY(session.windowMinimized());
        QTest::qWait(1500);
        QVERIFY(session.runLua(qsl(R"lua(echo("printed by a script\n"))lua")));
        session.host()->mAlertOnNewData = false;
        game->write("not asked for\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(texts(output).contains(qsl("not asked for")), 10000);
        QTest::qWait(100);
        QVERIFY(attention.isEmpty());

        // Two prompts ended with a go-ahead in one read, then another read
        session.host()->mAlertOnNewData = true;
        game->write("You hit.\r\n> \xff\xf9The orc hits.\r\n> \xff\xf9");
        QTRY_COMPARE_WITH_TIMEOUT(attention.size(), 1, 10000);
        QCOMPARE(attention.constFirst().at(0).toString(), name);
        QCOMPARE(attention.constFirst().at(1).toInt(), 0);
        QTRY_VERIFY_WITH_TIMEOUT(texts(output).contains(qsl("The orc hits.")), 10000);
        game->write("four\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(texts(output).contains(qsl("four")), 10000);
        QTest::qWait(100);
        QCOMPARE(attention.size(), 1);

        session.setWindowMinimized(false);
        game->write("restored\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(texts(output).contains(qsl("restored")), 10000);
        QTest::qWait(100);
        QCOMPARE(attention.size(), 1);
        session.setWindowMinimized(true);
        game->write("five\r\n");
        QTRY_COMPARE_WITH_TIMEOUT(attention.size(), 2, 10000);

        session.setWindowMinimized(false);
        QVERIFY(session.setApplicationActive(true));
        game->write("back in front\r\n");
        QTRY_VERIFY_WITH_TIMEOUT(texts(output).contains(qsl("back in front")), 10000);
        QTest::qWait(100);
        QCOMPARE(attention.size(), 2);
        QVERIFY(session.shutdown(false));
    }
};

#include "HeadlessSessionCompletionTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionCompletionTest)
