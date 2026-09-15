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

// Everything TCommandLine::event() does with a key press: sending a command,
// walking the history, completing from the game's output and from the
// registered suggestions, and the editing keys it has to intercept to keep its
// completion state honest.
//
// None of it is reachable from a Lua spec. Lua can put text into a command line
// and read it back, but it has no way to press a key in one - which is exactly
// why UI_spec.lua:6734 ("the callback only fires on a typed Enter"),
// GeyserCommandLine_spec.lua:152 and GeyserMiniConsole_spec.lua:294 are marked
// pending. Every branch below hangs off a QKeyEvent arriving at the widget.
//
// A fresh sub command line per test rather than the profile's main one: the
// history list is private and has no reset, so a shared command line would
// make each test's history depend on the ones that ran before it.

#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <chrono>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "ProfileTestHelper.h"
#include "RecordingTelnetServer.h"
#include "TCommandLine.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "ctelnet.h"
#include "mudlet.h"
#include "utils.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class CommandLineKeyHandlingTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    RecordingTelnetServer* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Test-CommandLine-Keys");
    const QString mLocalhost = qsl("localhost");
    int mLineCounter = 0;
    QString mLineName;

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    // A command line of this test's own, with an empty history and no
    // suggestions. Lua cannot delete one again, but it is parented into the
    // console so the profile's teardown takes it with it.
    TCommandLine* freshCommandLine()
    {
        mLineName = qsl("keyHandlingLine%1").arg(++mLineCounter);
        auto [created, message] = mpHost->mpConsole->createCommandLine(QString(), mLineName, 0, 0, 300, 30);
        if (!created) {
            qWarning() << "CommandLineKeyHandlingTest - could not create a command line:" << message;
            return nullptr;
        }
        TCommandLine* pCommandLine = mpHost->mpConsole->subCommandLineWidget(mLineName);
        if (pCommandLine) {
            pCommandLine->mSaveCommands = false;
        }
        return pCommandLine;
    }

    static void type(TCommandLine* pCommandLine, const QString& text) { QTest::keyClicks(pCommandLine, text); }

    // Real macOS arrow keys arrive with the keypad modifier set, and TCommandLine's
    // Up/Down handlers only recognise them there when it is present, so a synthesised
    // event has to carry it too or the key falls through to the editor.
    static void press(TCommandLine* pCommandLine, const Qt::Key key, const Qt::KeyboardModifiers modifiers = Qt::NoModifier)
    {
        Qt::KeyboardModifiers sentModifiers = modifiers;
#if defined(Q_OS_MACOS)
        if (key == Qt::Key_Up || key == Qt::Key_Down) {
            sentModifiers |= Qt::KeypadModifier;
        }
#endif
        QTest::keyClick(pCommandLine, key, sentModifiers);
    }

    static QString selection(const TCommandLine* pCommandLine) { return pCommandLine->textCursor().selectedText(); }

    // Puts text into the command line without going through the keys, for the
    // cases whose subject is what a key does to text that is already there - a
    // pasted line feed is not something QTest::keyClicks can type.
    static void setText(TCommandLine* pCommandLine, const QString& text)
    {
        pCommandLine->setPlainText(text);
        pCommandLine->moveCursor(QTextCursor::End);
    }

    // The bytes a run of commands makes on the wire, so a test can tell one
    // command carrying a line feed from two separate commands - the console
    // echo cannot, since it shows the same thing either way.
    QByteArray asSent(const QStringList& commands) const
    {
        const QByteArray eol = mpHost->mUSE_UNIX_EOL ? QByteArrayLiteral("\n") : QByteArrayLiteral("\r\n");
        QByteArray wire;
        for (const QString& command : commands) {
            wire += command.toUtf8() + eol;
        }
        return wire;
    }

    // Sends a command and puts the history position back to the newest end.
    // Sending leaves it one step in, on the command just sent, and typing the
    // next command is what normally puts it back - Escape is the other way, and
    // needs no text of its own that a later recall could then find.
    void sendCommand(TCommandLine* pCommandLine, const QString& command)
    {
        setText(pCommandLine, QString());
        type(pCommandLine, command);
        press(pCommandLine, Qt::Key_Return);
        press(pCommandLine, Qt::Key_Escape);
        setText(pCommandLine, QString());
    }

    bool runLua(const QString& script) { return mpHost->mLuaInterpreter.compileAndExecuteScript(script); }

    // What a script would see, so a callback that never ran is an empty string
    // rather than a stale one - every test that reads this clears it first.
    QString luaGlobal(const char* name) const
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        lua_getglobal(L, name);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
    }

    bool waitForServerToReceive(const QByteArray& text) const
    {
        return QTest::qWaitFor(
                [this, &text]() {
                    return mpServer->received().contains(text);
                },
                5000);
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

        mpServer = new RecordingTelnetServer(qApp);
        QVERIFY2(mpServer->start(), "RecordingTelnetServer failed to bind a loopback port");

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "Could not create the test profile - see the warning above for the step that timed out.");

        QSignalSpy connected(&mpHost->mTelnet, &cTelnet::signal_connected);
        if (connected.isEmpty()) {
            QVERIFY2(connected.wait(15s), "The test profile never connected to the recording server.");
        }
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // Both preferences change what a key press does rather than only what it
    // looks like, so each test starts from the shipped defaults.
    void init()
    {
        QVERIFY(mpHost);
        mpHost->mAutoClearCommandLineAfterSend = false;
        mpHost->mHighlightHistory = true;
        mpServer->forgetReceived();
    }

    void cleanup()
    {
        if (!mLineName.isEmpty()) {
            mpHost->resetCmdLineAction(mLineName);
            mLineName.clear();
        }
    }

    // The floor the rest of the file stands on: without this, a command line
    // that quietly ignored every key press would still satisfy assertions that
    // only ever look at what was sent.
    void test_typedCharactersReachTheDocument()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);
        QVERIFY(pCommandLine->toPlainText().isEmpty());

        type(pCommandLine, qsl("say hello"));

        QCOMPARE(pCommandLine->toPlainText(), qsl("say hello"));
    }

    // Return with no modifiers is what puts a command on the wire.
    void test_returnSendsWhatWasTyped()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);
        QSignalSpy submitted(pCommandLine, &TCommandLine::commandSubmitted);

        type(pCommandLine, qsl("look north"));
        QVERIFY2(!mpServer->received().contains("look north"), "the command reached the game before Return was pressed");

        press(pCommandLine, Qt::Key_Return);

        QCOMPARE(submitted.count(), 1);
        QVERIFY2(waitForServerToReceive("look north"), qPrintable(qsl("the game never received the command - it got: %1").arg(QString::fromUtf8(mpServer->received()))));
    }

    // The keypad's Enter arrives with the keypad modifier set and has to do the
    // same thing as the main Return.
    void test_keypadEnterSendsTheCommandToo()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        type(pCommandLine, qsl("kneel"));
        press(pCommandLine, Qt::Key_Enter, Qt::KeypadModifier);

        QVERIFY2(waitForServerToReceive("kneel"), qPrintable(qsl("the game never received the command - it got: %1").arg(QString::fromUtf8(mpServer->received()))));
    }

    // A pasted block of several lines is several commands, not one. Sent as a
    // single command it would lose the line feed on the way past
    // cTelnet::sendData, so the game would be asked to run the two lines run
    // together as one word.
    void test_returnSendsEachLineAsItsOwnCommand()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        setText(pCommandLine, qsl("firstofthepair\nsecondofthepair"));
        press(pCommandLine, Qt::Key_Return);

        QVERIFY2(waitForServerToReceive(asSent({qsl("firstofthepair"), qsl("secondofthepair")})),
                 qPrintable(qsl("the two lines did not reach the game as two commands - the wire holds %1").arg(QString::fromUtf8(mpServer->received().toPercentEncoding()))));
    }

    // Ctrl+Return scrolls the console back to the bottom, and must not also send
    void test_ctrlReturnDoesNotSendTheCommand()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        type(pCommandLine, qsl("neversentcommand"));
        press(pCommandLine, Qt::Key_Return, Qt::ControlModifier);
        QCOMPARE(pCommandLine->toPlainText(), qsl("neversentcommand"));

        // A command that does go, so that by the time the wire is read it has
        // had every chance to carry the one before it as well
        sendCommand(pCommandLine, qsl("commandsentafterwards"));
        QVERIFY2(waitForServerToReceive("commandsentafterwards"), "the command sent afterwards never arrived, so nothing can be concluded about the one before it");

        QVERIFY2(!mpServer->received().contains(QByteArrayLiteral("neversentcommand")), "Ctrl+Return sent the command as well as scrolling to the bottom");
    }

    // UI_spec.lua:6734 and GeyserCommandLine_spec.lua:152: setCmdLineAction
    // redirects a command line away from the game and into a Lua function, and
    // only a typed Enter makes that happen.
    void test_returnRunsTheCommandLineActionInsteadOfSending()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        QVERIFY(runLua(qsl("cmdLineActionSaw = ''")));
        QVERIFY(runLua(qsl("setCmdLineAction('%1', function(text) cmdLineActionSaw = text end)").arg(mLineName)));
        QVERIFY2(pCommandLine->mActionFunction, "setCmdLineAction did not reach the command line, so the rest of this proves nothing");

        type(pCommandLine, qsl("into the action"));
        QCOMPARE(luaGlobal("cmdLineActionSaw"), QString());

        press(pCommandLine, Qt::Key_Return);

        QCOMPARE(luaGlobal("cmdLineActionSaw"), qsl("into the action"));
        // Two escapes rather than one: the action replaces the send, it does not
        // run alongside it.
        QTest::qWait(200ms);
        QVERIFY2(!mpServer->received().contains("into the action"), "the command went to the game as well as to the action");
    }

    // resetCmdLineAction hands the command line back to the game.
    void test_resettingTheActionSendsToTheGameAgain()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        QVERIFY(runLua(qsl("cmdLineActionSaw = ''")));
        QVERIFY(runLua(qsl("setCmdLineAction('%1', function(text) cmdLineActionSaw = text end)").arg(mLineName)));
        QVERIFY(runLua(qsl("resetCmdLineAction('%1')").arg(mLineName)));
        QCOMPARE(pCommandLine->mActionFunction, 0);

        type(pCommandLine, qsl("back to the game"));
        press(pCommandLine, Qt::Key_Return);

        QCOMPARE(luaGlobal("cmdLineActionSaw"), QString());
        QVERIFY2(waitForServerToReceive("back to the game"), qPrintable(qsl("the game never received the command - it got: %1").arg(QString::fromUtf8(mpServer->received()))));
    }

    // Shift+Return is the multi-line escape hatch: it adds a line rather than
    // sending, and the whole lot goes out one command per line afterwards.
    void test_shiftReturnAddsALineInsteadOfSending()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        type(pCommandLine, qsl("north"));
        press(pCommandLine, Qt::Key_Return, Qt::ShiftModifier);
        type(pCommandLine, qsl("east"));

        QCOMPARE(pCommandLine->document()->blockCount(), 2);
        QVERIFY2(!mpServer->received().contains("north"), "Shift+Return sent the line instead of adding to it");

        press(pCommandLine, Qt::Key_Return);

        QVERIFY2(waitForServerToReceive("north"), "the first of the two lines never reached the game");
        QVERIFY2(waitForServerToReceive("east"), "the second of the two lines never reached the game");
    }

    // The preference decides what is left in the box afterwards: cleared, or
    // kept and selected so the next thing typed replaces it.
    void test_autoClearPreferenceDecidesWhatIsLeftBehind()
    {
        mpHost->mAutoClearCommandLineAfterSend = false;
        TCommandLine* pKept = freshCommandLine();
        QVERIFY(pKept);
        type(pKept, qsl("kept"));
        press(pKept, Qt::Key_Return);
        QCOMPARE(pKept->toPlainText(), qsl("kept"));
        QCOMPARE(selection(pKept), qsl("kept"));

        mpHost->mAutoClearCommandLineAfterSend = true;
        TCommandLine* pCleared = freshCommandLine();
        QVERIFY(pCleared);
        type(pCleared, qsl("cleared"));
        press(pCleared, Qt::Key_Return);
        QCOMPARE(pCleared->toPlainText(), QString());
    }

    // Up walks back through what was sent, Down comes forward again, and the
    // walk stops at the oldest entry rather than running off the end.
    void test_upAndDownWalkTheCommandHistory()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        type(pCommandLine, qsl("alpha"));
        press(pCommandLine, Qt::Key_Return);
        type(pCommandLine, qsl("beta"));
        press(pCommandLine, Qt::Key_Return);

        press(pCommandLine, Qt::Key_Up);
        QCOMPARE(pCommandLine->toPlainText(), qsl("alpha"));
        press(pCommandLine, Qt::Key_Up);
        QCOMPARE(pCommandLine->toPlainText(), qsl("alpha"));

        press(pCommandLine, Qt::Key_Down);
        QCOMPARE(pCommandLine->toPlainText(), qsl("beta"));
    }

    // Down on freshly typed text banks it into the history and clears the line -
    // the "I'll come back to this" gesture.
    void test_downOnFreshTextBanksItAndClearsTheLine()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        type(pCommandLine, qsl("half written"));
        press(pCommandLine, Qt::Key_Down);

        QCOMPARE(pCommandLine->toPlainText(), QString());
        QVERIFY2(!mpServer->received().contains("half written"), "banking the line sent it to the game");

        press(pCommandLine, Qt::Key_Up);
        QCOMPARE(pCommandLine->toPlainText(), qsl("half written"));
    }

    // With only part of the line selected, Up completes from the history rather
    // than replacing the line: the matched entry is filled in and the part the
    // user did not type is left selected, so typing on replaces it.
    void test_upCompletesFromTheHistoryWhenTheLineIsOnlyPartlySelected()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        type(pCommandLine, qsl("hello world"));
        press(pCommandLine, Qt::Key_Return);
        // Return leaves the whole line selected, so typing replaces it and
        // leaves nothing selected - which is the state this branch needs.
        type(pCommandLine, qsl("hel"));
        QCOMPARE(pCommandLine->toPlainText(), qsl("hel"));
        QVERIFY(selection(pCommandLine).isEmpty());

        press(pCommandLine, Qt::Key_Up);

        QCOMPARE(pCommandLine->toPlainText(), qsl("hello world"));
        QCOMPARE(selection(pCommandLine), qsl("lo world"));
    }

    // A password typed at a game's login prompt arrives with remote echo on, and
    // must not be left in a history the next player at the keyboard can page
    // through.
    void test_aPasswordIsNotKeptInTheHistory()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);
        sendCommand(pCommandLine, qsl("ordinarycommandbefore"));

        mpHost->setRemoteEchoingActive(true);
        sendCommand(pCommandLine, qsl("hunter2secret"));
        mpHost->setRemoteEchoingActive(false);

        press(pCommandLine, Qt::Key_Up);

        QVERIFY2(pCommandLine->toPlainText() != qsl("hunter2secret"), "the password typed at the game's prompt was kept in the command history");
        QCOMPARE(pCommandLine->toPlainText(), qsl("ordinarycommandbefore"));
    }

    // Tab completes the word being typed from what the game has said recently,
    // and pressing it again cycles on to the next match.
    void test_tabCompletesAWordFromTheConsoleBuffer()
    {
        mpHost->mpConsole->print(qsl("qzxalpha qzxbravo\n"));
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        type(pCommandLine, qsl("qzx"));
        press(pCommandLine, Qt::Key_Tab);
        const QString first = pCommandLine->toPlainText();
        QVERIFY2(first == qsl("qzxalpha") || first == qsl("qzxbravo"), qPrintable(qsl("Tab completed to '%1' rather than to either of the words in the buffer").arg(first)));

        press(pCommandLine, Qt::Key_Tab);
        const QString second = pCommandLine->toPlainText();
        QVERIFY2(second == qsl("qzxalpha") || second == qsl("qzxbravo"), qPrintable(qsl("a second Tab left '%1', which is neither of the words in the buffer").arg(second)));
        QVERIFY2(second != first, "a second Tab did not cycle on to the other match");

        // Backtab is the same cycle in reverse
        press(pCommandLine, Qt::Key_Backtab, Qt::ShiftModifier);
        QCOMPARE(pCommandLine->toPlainText(), first);
    }

    // Only the word under the cursor is replaced - the words accepted before it
    // are already what the player meant.
    void test_tabCompletesOnlyTheWordBeingTyped()
    {
        mpHost->mpConsole->print(qsl("a qzxquinquagenarian appears\n"));
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        type(pCommandLine, qsl("greet qzxquinq"));
        press(pCommandLine, Qt::Key_Tab);

        QCOMPARE(pCommandLine->toPlainText(), qsl("greet qzxquinquagenarian"));
    }

    // There is no part-word to complete after a space, and guessing one from the
    // word before it would overwrite what was already accepted.
    void test_tabDoesNothingAfterASpace()
    {
        mpHost->mpConsole->print(qsl("the qzxbrachiosaurus lumbers past\n"));
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        type(pCommandLine, qsl("qzxbrachiosaurus "));
        press(pCommandLine, Qt::Key_Tab);

        QCOMPARE(pCommandLine->toPlainText(), qsl("qzxbrachiosaurus "));
    }

    // Typing a space accepts the completion. A Tab straight after it must not
    // carry on cycling through the matches and swap the accepted word for the
    // other one - which is what it would do if the space left the cycle where
    // it was, since a space is the one key that goes past the typing tracker.
    // Which of the two matches comes first is not part of the promise, so the
    // test takes whichever Tab offered.
    void test_aSpaceAcceptsTheCompletionSoTabNoLongerCyclesIt()
    {
        mpHost->mpConsole->print(qsl("qzxobstreperous qzxobfuscatory\n"));
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        type(pCommandLine, qsl("qzxob"));
        press(pCommandLine, Qt::Key_Tab);
        const QString first = pCommandLine->toPlainText();
        QVERIFY2(first == qsl("qzxobstreperous") || first == qsl("qzxobfuscatory"), qPrintable(qsl("Tab completed to '%1' rather than to either of the words in the buffer").arg(first)));

        press(pCommandLine, Qt::Key_Space);
        press(pCommandLine, Qt::Key_Tab);

        QCOMPARE(pCommandLine->toPlainText(), first + QChar::Space);
    }

    // Once a completion has been accepted, a fresh part-word typed after it
    // starts a new completion from the first match rather than carrying on
    // from wherever the last one was.
    void test_aNewPartWordStartsTheCompletionOver()
    {
        mpHost->mpConsole->print(qsl("qzxobstreperous qzxobfuscatory\n"));
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        type(pCommandLine, qsl("qzxob"));
        press(pCommandLine, Qt::Key_Tab);
        const QString first = pCommandLine->toPlainText();
        QVERIFY2(first == qsl("qzxobstreperous") || first == qsl("qzxobfuscatory"), qPrintable(qsl("Tab completed to '%1' rather than to either of the words in the buffer").arg(first)));

        setText(pCommandLine, QString());
        type(pCommandLine, qsl("say "));
        type(pCommandLine, qsl("qzxob"));
        press(pCommandLine, Qt::Key_Tab);

        QCOMPARE(pCommandLine->toPlainText(), qsl("say %1").arg(first));
    }

    // addSuggestion puts a word into the completion pool that the game never
    // said, and clearSuggestions takes the whole pool away again.
    void test_tabCompletesRegisteredSuggestionsUntilTheyAreCleared()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);
        pCommandLine->addSuggestion(qsl("qzxsuggested"));

        type(pCommandLine, qsl("qzxsug"));
        press(pCommandLine, Qt::Key_Tab);
        QCOMPARE(pCommandLine->toPlainText(), qsl("qzxsuggested"));

        TCommandLine* pAfterClearing = freshCommandLine();
        QVERIFY(pAfterClearing);
        pAfterClearing->addSuggestion(qsl("qzxsuggested"));
        pAfterClearing->clearSuggestions();
        type(pAfterClearing, qsl("qzxsug"));
        press(pAfterClearing, Qt::Key_Tab);
        QCOMPARE(pAfterClearing->toPlainText(), qsl("qzxsug"));
    }

    // A blacklisted word is dropped from the pool even though it is right there
    // in the game's output, and taking it off the blacklist puts it back.
    void test_blacklistedWordsAreNeverOffered()
    {
        mpHost->mpConsole->print(qsl("qzxkeepme qzxdropme\n"));
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);
        pCommandLine->addBlacklist(qsl("qzxdropme"));

        // A prefix both words in the buffer match, so cycling has somewhere to
        // go and skipping the blacklisted one is a choice rather than a refusal
        type(pCommandLine, qsl("qzx"));
        for (int press_ = 0; press_ < 4; ++press_) {
            press(pCommandLine, Qt::Key_Tab);
            QVERIFY2(pCommandLine->toPlainText() != qsl("qzxdropme"), "Tab offered a blacklisted word");
        }

        pCommandLine->clear();
        type(pCommandLine, qsl("qzxdrop"));
        press(pCommandLine, Qt::Key_Tab);
        QCOMPARE(pCommandLine->toPlainText(), qsl("qzxdrop"));

        // The same command line, because the blacklist is a member of it: a
        // fresh one starts with an empty blacklist and would complete whether
        // or not removeBlacklist() did anything.
        pCommandLine->removeBlacklist(qsl("qzxdropme"));
        pCommandLine->clear();
        type(pCommandLine, qsl("qzxdrop"));
        press(pCommandLine, Qt::Key_Tab);
        QCOMPARE(pCommandLine->toPlainText(), qsl("qzxdropme"));
    }

    // Escape leaves completion mode, and selects the line so the next thing
    // typed replaces it.
    void test_escapeSelectsTheWholeLine()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        type(pCommandLine, qsl("abandon this"));
        QVERIFY(selection(pCommandLine).isEmpty());

        press(pCommandLine, Qt::Key_Escape);

        QCOMPARE(selection(pCommandLine), qsl("abandon this"));
    }

    // Backspace and Delete are intercepted rather than left to QPlainTextEdit,
    // so that the completion state shrinks along with the text - but the text
    // still has to shrink.
    void test_backspaceAndDeleteStillEditTheLine()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        type(pCommandLine, qsl("abc"));
        press(pCommandLine, Qt::Key_Backspace);
        QCOMPARE(pCommandLine->toPlainText(), qsl("ab"));

        pCommandLine->moveCursor(QTextCursor::Start);
        press(pCommandLine, Qt::Key_Delete);
        QCOMPARE(pCommandLine->toPlainText(), qsl("b"));
    }

    // Ctrl+Up and Ctrl+Down move the caret inside a multi-line command instead
    // of walking the history away from under it.
    void test_ctrlUpAndDownMoveTheCaretRatherThanTheHistory()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        type(pCommandLine, qsl("first"));
        press(pCommandLine, Qt::Key_Return);
        type(pCommandLine, qsl("one"));
        press(pCommandLine, Qt::Key_Return, Qt::ShiftModifier);
        type(pCommandLine, qsl("two"));
        const int lastBlock = pCommandLine->textCursor().blockNumber();
        QCOMPARE(lastBlock, 1);

        press(pCommandLine, Qt::Key_Up, Qt::ControlModifier);

        QCOMPARE(pCommandLine->textCursor().blockNumber(), 0);
        QVERIFY2(pCommandLine->toPlainText() != qsl("first"), "Ctrl+Up walked the history instead of moving the caret");

        press(pCommandLine, Qt::Key_Down, Qt::ControlModifier);
        QCOMPARE(pCommandLine->textCursor().blockNumber(), 1);
    }
};

#include "CommandLineKeyHandlingTest.moc"
MUDLET_GROUPED_TEST_MAIN(CommandLineKeyHandlingTest)
