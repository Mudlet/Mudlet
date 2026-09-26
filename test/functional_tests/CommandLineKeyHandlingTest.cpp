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

#include <QClipboard>
#include <QFileInfo>
#include <QLineEdit>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <chrono>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "ProfileTestHelper.h"
#include "RecordingTelnetServer.h"
#include "KeyUnit.h"
#include "TCommandLine.h"
#include "ctelnet.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TUiTour.h"
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
    QStringList mPermKeyNames;

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    // The profile's own command line. freshCommandLine() below makes a
    // SubCommandLine, and setEchoSuppression() returns early for anything that is
    // not a MainCommandLine, so only this one is ever masked. It belongs to the
    // profile rather than to a case, so it starts from a known empty line.
    TCommandLine* mainCommandLine() const
    {
        TCommandLine* pCommandLine = mpHost->mpConsole->mpCommandLine;
        if (pCommandLine) {
            pCommandLine->mSaveCommands = false;
            pCommandLine->clear();
        }
        return pCommandLine;
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

    // Text from the game, through the real telnet parser rather than printed
    // straight into the console: what the command line remembers about itself is
    // keyed off the game having said something, and only that path says so. Ends
    // in a newline so the line is posted at once instead of held as a prompt.
    void serverSays(const char* text)
    {
        QByteArray bytes(text);
        mpHost->mTelnet.loopbackTest(bytes);
    }

    // The game taking or releasing ECHO, through the real telnet parser rather
    // than by setting the Host flag, so that everything the negotiation does on
    // the way - including flushing text it was holding - happens as it does in
    // play.
    void serverEcho(bool takesEcho)
    {
        QByteArray bytes;
        bytes.append(TN_IAC).append(takesEcho ? TN_WILL : TN_WONT).append(OPT_ECHO);
        mpHost->mTelnet.loopbackTest(bytes);
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

    // A permanent key cannot be deleted from Lua, only switched off, and one
    // left active on a plain letter would eat that letter in every test that
    // runs after it.
    void switchOffAfterwards(const QStringList& names) { mPermKeyNames << names; }

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
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        // A config dir of this test's own reads as a brand new installation, so
        // the first-run interface tour would open a second after the profile
        // loads and its application-wide event filter would swallow every key
        // aimed at the main window - silently, for as many slots as the tour
        // stays up. Written before init(), which is what stamps an untouched
        // config as a first launch: a settings file that already holds
        // something is how mudletUsedBefore() recognises an existing player,
        // which keeps the rest of the first-run interface away as well.
        TUiTour::rememberShown();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QVERIFY2(mudlet::self()->experiencedMudletPlayer(), "the first-run UI would open over these tests and eat their key presses");
        QDir(MudletApp::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

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
        // Null when initTestCase skipped or failed ahead of mudlet::start()
        if (mudlet::self()) {
            QDir(MudletApp::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();
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
        // Per-Host and shared across this class's cases, so a case that aborts with
        // the prompt still open would mask the ones after it. Not inline in the
        // cases: QTest abandons a slot on a failed assertion, which is exactly when
        // this matters.
        mpHost->setRemoteEchoingActive(false);

        if (!mLineName.isEmpty()) {
            mpHost->resetCmdLineAction(mLineName);
            mLineName.clear();
        }
        for (const QString& name : mPermKeyNames) {
            mpHost->getKeyUnit()->disableKey(name);
        }
        mPermKeyNames.clear();
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

    // The word boundaries the completion works out, both in the game's output and
    // in what has been typed, have to be the Unicode ones, or a player of a game
    // that is not in English gets no completion past the first accented letter
    // (#1954)
    void test_tabCompletesPastANonAsciiLetter()
    {
        mpHost->mpConsole->print(qsl("qzvbjörnsson waves\n"));
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        type(pCommandLine, qsl("qzvbj"));
        // QTest's key helpers assert on anything outside ASCII, so the letter this
        // test is about has to be sent as the event a real keyboard produces
        QKeyEvent accentedLetter(QEvent::KeyPress, Qt::Key_Odiaeresis, Qt::NoModifier, qsl("ö"));
        QApplication::sendEvent(pCommandLine, &accentedLetter);
        QCOMPARE(pCommandLine->toPlainText(), qsl("qzvbjö"));

        press(pCommandLine, Qt::Key_Tab);

        QCOMPARE(pCommandLine->toPlainText(), qsl("qzvbjörnsson"));
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

    // A key the switch names but no binding matches has to fall through to
    // QPlainTextEdit rather than be swallowed as handled, or the editing shortcuts
    // the editor brings with it - Ctrl+Delete among them - stop working (#2755)
    void test_ctrlDeleteStillDeletesTheWordAhead()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        // which modifier deletes a word is the platform's to say, and macOS has no
        // binding for it at all, so ask rather than assume
        const QList<QKeySequence> deleteWordBindings = QKeySequence::keyBindings(QKeySequence::DeleteEndOfWord);
        if (deleteWordBindings.isEmpty()) {
            QSKIP("this platform binds no key to delete-end-of-word, so there is nothing to fall through to");
        }
        const QKeyCombination deleteWord = deleteWordBindings.first()[0];

        type(pCommandLine, qsl("keep this"));
        pCommandLine->moveCursor(QTextCursor::Start);

        press(pCommandLine, static_cast<Qt::Key>(deleteWord.key()), deleteWord.keyboardModifiers());

        QCOMPARE(pCommandLine->toPlainText(), qsl("this"));
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

    // Ctrl+C copies what is selected in the game window rather than what is on
    // the line, and the split-screen scrollback pane is as much the game window
    // as the pane above it - selecting there and pressing Ctrl+C used to copy
    // the command line instead (#8551)
    void test_copyTakesTheScrollbackPanesSelectionOverTheLine()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        TMainConsole* pConsole = mpHost->mpConsole;
        const QString sentinel = qsl("qzxscrollbackline");
        for (int line = 0; line < 60; ++line) {
            pConsole->print(qsl("%1 %2\n").arg(sentinel, QString::number(line)));
        }

        QClipboard* pClipboard = QApplication::clipboard();
        QVERIFY(pClipboard);
        pClipboard->setText(qsl("nothing has been copied yet"));
        // Selecting in the command line drops the console's selection, so the
        // contest has to be set up in this order to exist at all.
        type(pCommandLine, qsl("a command in the way"));
        pCommandLine->selectAll();

        // Scrolling back is what opens the lower pane; without it there is no
        // second pane to select in. A selection in a pane of no height cannot be
        // worked out, and the panes are only laid out once the window they are in
        // is up, so it has to be shown and sized first.
        const QSize windowSize = mudlet::self()->size();
        mudlet::self()->resize(1200, 800);
        mudlet::self()->show();
        const auto restoreTheWindow = qScopeGuard([pConsole, windowSize]() {
            pConsole->scrollDown(100);
            // scrolling back down hides the lower pane but leaves it selected, for
            // the next test that presses Ctrl+C to copy; blanking mSelectedRegion
            // is not enough, the buffer cells stay flagged until unHighlight()
            pConsole->clearSelection();
            mudlet::self()->hide();
            mudlet::self()->resize(windowSize);
        });
        QVERIFY2(QTest::qWaitForWindowExposed(mudlet::self()), "the main window never came up");
        pConsole->scrollUp(30);
        // the upper pane's half of the scroll runs on a 0ms timer, and the lower
        // pane only gets a height once the layout has run, so neither is true yet
        QTRY_VERIFY2(!pConsole->mUpperPane->mIsTailMode, "the console never actually scrolled back");
        QTRY_VERIFY2(pConsole->mLowerPane->isVisible() && pConsole->mLowerPane->height() > 0, "scrolling back did not open the split-screen scrollback");

        pConsole->mLowerPane->slot_selectAll();
        // the upper pane is asked first, so it has to be out of the running for
        // this to be about the lower one at all
        QVERIFY2(pConsole->mUpperPane->mSelectedRegion.isEmpty(), "the upper pane holds a selection, so a copy from it would prove nothing about the lower one");
        QVERIFY2(!pConsole->mLowerPane->mSelectedRegion.isEmpty(), "select-all put no selection on the scrollback pane");
        QVERIFY2(pCommandLine->textCursor().hasSelection(), "the command line lost the selection it is meant to lose the contest with");

        press(pCommandLine, Qt::Key_C, Qt::ControlModifier);

        QVERIFY2(pClipboard->text().contains(sentinel), qPrintable(qsl("Ctrl+C copied '%1' rather than the scrollback pane's selection").arg(pClipboard->text().left(60))));
    }

    // #10764: a key put in a group made by permGroup(name, "key") reported
    // itself active and still never fired, because the group above it was
    // created switched off and KeyUnit never descends into an inactive folder.
    // Only a real key press shows that, which is why this lives here and not in
    // KeyBinds_spec.lua - Lua can read a key's state but cannot press one.
    //
    // permGroup() itself is mudlet-lua, which a functional test profile does not
    // load, so the group is made the way permGroup(name, "key") makes it: the
    // keycode of -1 that permKey() passes on for a folder. Other_spec.lua pins
    // that permGroup still dispatches to permKey(name, parent, -1, "").
    void test_aKeyInAFreshPermGroupFires()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);
        QVERIFY(runLua(qsl("keyGroupFired = ''")));
        QString group = qsl("keyGroupSpecGroup");
        QString key = qsl("keyGroupSpecKey");
        QString noParent;
        QString noScript;
        QString keyScript = qsl("keyGroupFired = 'yes'");
        int folderKeycode = -1;
        int noModifier = Qt::NoModifier;
        int letterJ = Qt::Key_J;

        auto [groupId, groupMessage] = mpHost->mLuaInterpreter.startPermKey(group, noParent, folderKeycode, noModifier, noScript);
        QVERIFY2(groupId > 0, qPrintable(groupMessage));
        auto [keyId, keyMessage] = mpHost->mLuaInterpreter.startPermKey(key, group, letterJ, noModifier, keyScript);
        QVERIFY2(keyId > 0, qPrintable(keyMessage));
        switchOffAfterwards({key, group});

        press(pCommandLine, Qt::Key_J);

        QCOMPARE(luaGlobal("keyGroupFired"), qsl("yes"));
        // and the press belongs to the binding now: TCommandLine::event() takes
        // a match as handled, so the character it was typed with never reaches
        // the document - a binding on a plain letter costs the user that letter
        QCOMPARE(pCommandLine->toPlainText(), QString());
    }

    // The control for the test above: a key that fired whatever state the group
    // above it was in would pass that one just as well.
    void test_aKeyInASwitchedOffPermGroupDoesNotFire()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);

        QVERIFY(runLua(qsl("keyGroupFired = ''")));
        QString group = qsl("keyGroupSpecOffGroup");
        QString key = qsl("keyGroupSpecOffKey");
        QString noParent;
        QString noScript;
        QString keyScript = qsl("keyGroupFired = 'yes'");
        int folderKeycode = -1;
        int noModifier = Qt::NoModifier;
        int letterK = Qt::Key_K;

        auto [groupId, groupMessage] = mpHost->mLuaInterpreter.startPermKey(group, noParent, folderKeycode, noModifier, noScript);
        QVERIFY2(groupId > 0, qPrintable(groupMessage));
        auto [keyId, keyMessage] = mpHost->mLuaInterpreter.startPermKey(key, group, letterK, noModifier, keyScript);
        QVERIFY2(keyId > 0, qPrintable(keyMessage));
        switchOffAfterwards({key, group});
        QVERIFY(mpHost->getKeyUnit()->disableKey(group));

        press(pCommandLine, Qt::Key_K);

        QCOMPARE(luaGlobal("keyGroupFired"), QString());
        // nothing claimed the press, so it is the command line's again
        QCOMPARE(pCommandLine->toPlainText(), qsl("k"));
    }

    // Ctrl+F opens the console's search bar, but a player who had already bound
    // it themselves keeps their binding - it is asked first (#6694)
    void test_aCtrlFBindingRunsInsteadOfOpeningTheSearchBar()
    {
        TCommandLine* pCommandLine = freshCommandLine();
        QVERIFY(pCommandLine);
        QVERIFY(runLua(qsl("ctrlFKeyFired = ''")));
        QString key = qsl("ctrlFSpecKey");
        QString noParent;
        QString keyScript = qsl("ctrlFKeyFired = 'yes'");
        int letterF = Qt::Key_F;
        int controlModifier = Qt::ControlModifier;

        auto [keyId, keyMessage] = mpHost->mLuaInterpreter.startPermKey(key, noParent, letterF, controlModifier, keyScript);
        QVERIFY2(keyId > 0, qPrintable(keyMessage));
        switchOffAfterwards({key});

        // opening the search bar selects whatever is in it, so a search bar that
        // was left deselected and comes back selected is one that opened
        QLineEdit* pSearchBox = mpHost->mpConsole->mpBufferSearchBox;
        QVERIFY(pSearchBox);
        pSearchBox->setText(qsl("a search that was already there"));
        pSearchBox->deselect();

        press(pCommandLine, Qt::Key_F, Qt::ControlModifier);

        QCOMPARE(luaGlobal("ctrlFKeyFired"), qsl("yes"));
        QVERIFY2(!pSearchBox->hasSelectedText(), "the search bar opened as well as the binding running");
    }

    // What a player had typed before the game asked for a password is a command;
    // what they typed after seeing the prompt could be the password. Neither the
    // history nor a clock can tell those apart - the boundary is the game's own
    // output, so the command line remembers what it held each time the game spoke,
    // and the prompt splits the line there.
    //
    // These assert on the command line rather than on what the game received: an
    // empty masked line is already proof that nothing on it can go out with the
    // password, and reading the wire needs waitForServerToReceive(), which spins
    // the event loop - after which synthetic key events stop reaching the command
    // line. So exactly one case reads the wire, as the last thing it does, and it
    // is declared last. Nothing may type after a wait, here or in a later case.
    //
    // They also drive the profile's own command line, the only one that is ever
    // masked, and leave entries in its history, which nothing resets - the other
    // reason this group is last.
    void test_aCommandTypedBeforeThePromptIsParkedAndGivenBack()
    {
        TCommandLine* pCommandLine = mainCommandLine();
        QVERIFY(pCommandLine);
        QCOMPARE(pCommandLine->getType(), TCommandLine::MainCommandLine);
        QVERIFY2(!mpHost->mDisablePasswordMasking, "password masking is off in this profile, so this case proves nothing");

        type(pCommandLine, qsl("qzxleftover"));
        serverSays("Password:\n");

        mpHost->setRemoteEchoingActive(true);
        QVERIFY2(pCommandLine->toPlainText().isEmpty(), qPrintable(qsl("the command was carried into the password prompt, leaving '%1' on the masked line").arg(pCommandLine->toPlainText())));

        type(pCommandLine, qsl("qzxsecret"));
        // The game talks while the password is being typed, so what the line is
        // remembered as holding is now the half-typed password. Harmless here, and
        // it is what makes the re-prompt below a real test of the restore.
        serverSays("You hear distant thunder.\n");
        press(pCommandLine, Qt::Key_Return);

        // #7921: the command comes back once the prompt is over, even though the
        // player answered the prompt by hand.
        mpHost->setRemoteEchoingActive(false);
        QCOMPARE(pCommandLine->toPlainText(), qsl("qzxleftover"));

        // A rejected password brings the prompt straight back, and a real prompt
        // has no newline, so nothing is posted between the game releasing ECHO and
        // taking it again. The command was put back by the game releasing ECHO, so
        // it has to count as predating this prompt, not as something typed into it -
        // and the last text the game posted was the thunder, over a half-typed
        // password, so nothing but the restore itself can say so.
        mpHost->setRemoteEchoingActive(true);
        QVERIFY2(pCommandLine->toPlainText().isEmpty(), qPrintable(qsl("the restored command was kept on the masked line at the re-prompt, leaving '%1'").arg(pCommandLine->toPlainText())));
        // Answered by hand again. Only text the game is known to have seen comes
        // back after that, so this is where the restore's own refresh of what the
        // line held is load-bearing: without it the command would count as typed
        // after the thunder, and be dropped here.
        type(pCommandLine, qsl("qzxsecret"));
        press(pCommandLine, Qt::Key_Return);
        mpHost->setRemoteEchoingActive(false);
        QCOMPARE(pCommandLine->toPlainText(), qsl("qzxleftover"));
    }

    // The other half, which the fix must keep: a password typed in the gap between
    // the game printing its prompt and its WILL ECHO arriving is kept out of the
    // password only by being set aside, and is never handed back in the clear.
    void test_aPasswordTypedAfterThePromptIsNeverHandedBack()
    {
        TCommandLine* pCommandLine = mainCommandLine();
        QVERIFY(pCommandLine);

        serverSays("Password:\n");
        type(pCommandLine, qsl("qzxpassstart"));

        mpHost->setRemoteEchoingActive(true);
        QVERIFY(pCommandLine->toPlainText().isEmpty());
        type(pCommandLine, qsl("qzxrest"));
        press(pCommandLine, Qt::Key_Return);

        mpHost->setRemoteEchoingActive(false);
        QVERIFY2(pCommandLine->toPlainText().isEmpty(), qPrintable(qsl("password characters were handed back after the prompt - the line holds '%1'").arg(pCommandLine->toPlainText())));
    }

    // Text typed after the game last spoke could be a reply to a prompt the
    // player saw, or a command typed into a silence the game then broke with
    // WILL ECHO - the same order of events, which nothing on the line tells
    // apart. Answered by hand, and not a command the player has sent before, it
    // is dropped: never sent with the password, never shown afterwards.
    void test_textTypedIntoASilenceIsNeitherSentNorShownWhenThePromptIsAnsweredByHand()
    {
        TCommandLine* pCommandLine = mainCommandLine();
        QVERIFY(pCommandLine);

        serverSays("Checking the ledgers.\n");
        type(pCommandLine, qsl("qzxneversent"));

        mpHost->setRemoteEchoingActive(true);
        QVERIFY2(pCommandLine->toPlainText().isEmpty(), qPrintable(qsl("text typed into the silence was kept on the masked line, leaving '%1'").arg(pCommandLine->toPlainText())));

        type(pCommandLine, qsl("qzxsecret"));
        press(pCommandLine, Qt::Key_Return);

        mpHost->setRemoteEchoingActive(false);
        QVERIFY2(pCommandLine->toPlainText().isEmpty(),
                 qPrintable(qsl("text that may have been the start of a password was shown after the prompt - the line holds '%1'").arg(pCommandLine->toPlainText())));
    }

    // The same silence, answered by a script: nothing was typed at the prompt, so
    // the text cannot have been a password, and it comes back. This is what #7921
    // asked for, and one of the two shapes of it that can be told apart.
    void test_textTypedIntoASilenceComesBackWhenAScriptAnswersThePrompt()
    {
        TCommandLine* pCommandLine = mainCommandLine();
        QVERIFY(pCommandLine);

        serverSays("Checking the ledgers.\n");
        type(pCommandLine, qsl("qzxtypedahead"));

        mpHost->setRemoteEchoingActive(true);
        QVERIFY(pCommandLine->toPlainText().isEmpty());
        QVERIFY(runLua(qsl("send('qzxscriptedpassword', false)")));

        mpHost->setRemoteEchoingActive(false);
        QCOMPARE(pCommandLine->toPlainText(), qsl("qzxtypedahead"));
    }

    // The other shape: answered by hand, but the text is a command the player has
    // sent before. It is already in the history, in the clear, on disk - so giving
    // it back can show nothing the history does not hold, and it comes back.
    void test_textTypedIntoASilenceComesBackWhenItIsAlreadyInTheHistory()
    {
        TCommandLine* pCommandLine = mainCommandLine();
        QVERIFY(pCommandLine);
        sendCommand(pCommandLine, qsl("qzxlook"));
        // That it reached the history is the precondition, and Up is the way to
        // read it back without reaching into the widget.
        press(pCommandLine, Qt::Key_Up);
        QVERIFY2(pCommandLine->toPlainText() == qsl("qzxlook"), qPrintable(qsl("the command did not reach the history, so this case proves nothing - Up gave '%1'").arg(pCommandLine->toPlainText())));
        pCommandLine->clear();

        serverSays("Checking the ledgers.\n");
        type(pCommandLine, qsl("qzxlook"));

        mpHost->setRemoteEchoingActive(true);
        QVERIFY(pCommandLine->toPlainText().isEmpty());
        type(pCommandLine, qsl("qzxsecret"));
        press(pCommandLine, Qt::Key_Return);

        mpHost->setRemoteEchoingActive(false);
        QCOMPARE(pCommandLine->toPlainText(), qsl("qzxlook"));
    }

    // A real password prompt has no newline on the end, so the parser holds it
    // back waiting for one - and the WILL ECHO behind it can arrive in the same
    // read. The prompt has still been sent, so what was typed before it is a
    // command; without flushing that held text the command line would not know
    // the game had spoken, and would treat the command as something typed in
    // reply and drop it. Driven through the telnet parser, since it is the
    // negotiation itself that has to do the flushing.
    void test_anUnterminatedPromptStillMarksWhatWasTypedBeforeIt()
    {
        TCommandLine* pCommandLine = mainCommandLine();
        QVERIFY(pCommandLine);

        serverSays("Checking the ledgers.\n");
        // Never sent, so the history cannot be what brings it back.
        type(pCommandLine, qsl("qzxheldahead"));
        serverSays("Password: ");

        serverEcho(true);
        QVERIFY2(pCommandLine->toPlainText().isEmpty(), qPrintable(qsl("the command was carried into the password prompt, leaving '%1' on the masked line").arg(pCommandLine->toPlainText())));

        type(pCommandLine, qsl("qzxheldpass"));
        press(pCommandLine, Qt::Key_Return);

        serverEcho(false);
        QCOMPARE(pCommandLine->toPlainText(), qsl("qzxheldahead"));
    }

    // The other side of that flush: a read carrying only the negotiation, after
    // the prompt was shown and the player has started their password. Nothing is
    // held, so nothing has been said since they began typing - those characters
    // must stay uncertain and never come back in the clear.
    void test_aBareNegotiationDoesNotMarkAHalfTypedPassword()
    {
        TCommandLine* pCommandLine = mainCommandLine();
        QVERIFY(pCommandLine);

        serverSays("Password: ");
        type(pCommandLine, qsl("qzxbarestart"));

        serverEcho(true);
        QVERIFY(pCommandLine->toPlainText().isEmpty());
        type(pCommandLine, qsl("qzxbarerest"));
        press(pCommandLine, Qt::Key_Return);

        serverEcho(false);
        QVERIFY2(pCommandLine->toPlainText().isEmpty(), qPrintable(qsl("password characters came back after the prompt - the line holds '%1'").arg(pCommandLine->toPlainText())));
    }

    // A key that edits nothing is not the player answering the prompt. Pressing
    // Shift while a script logs in must not make the parked command look like
    // something typed in reply, which would discard it.
    void test_aModifierKeyAtThePromptDoesNotDiscardTheCommand()
    {
        TCommandLine* pCommandLine = mainCommandLine();
        QVERIFY(pCommandLine);

        serverSays("Checking the ledgers.\n");
        type(pCommandLine, qsl("qzxmodahead"));

        mpHost->setRemoteEchoingActive(true);
        QVERIFY(pCommandLine->toPlainText().isEmpty());
        press(pCommandLine, Qt::Key_Shift);
        QVERIFY(runLua(qsl("send('qzxmodpassword', false)")));

        mpHost->setRemoteEchoingActive(false);
        QCOMPARE(pCommandLine->toPlainText(), qsl("qzxmodahead"));
    }

    // Both at once: a command the game printed after, then more typed before WILL
    // ECHO. The line holds them as one string, and the game's output is the only
    // thing that says where the certain part ends. The certain part comes back;
    // the rest is answered by hand and never sent before, so it is dropped.
    //
    // Declared last, and reads the wire last, for the reason given above.
    void test_theLineIsSplitWhereTheGameLastSpoke()
    {
        TCommandLine* pCommandLine = mainCommandLine();
        QVERIFY(pCommandLine);

        type(pCommandLine, qsl("qzxsplitahead"));
        serverSays("Password:\n");
        type(pCommandLine, qsl("qzxspl"));

        mpHost->setRemoteEchoingActive(true);
        QVERIFY2(pCommandLine->toPlainText().isEmpty(), qPrintable(qsl("something was kept on the masked line, leaving '%1'").arg(pCommandLine->toPlainText())));

        mpServer->forgetReceived();
        type(pCommandLine, qsl("qzxsplitpass"));
        press(pCommandLine, Qt::Key_Return);
        mpHost->setRemoteEchoingActive(false);
        QCOMPARE(pCommandLine->toPlainText(), qsl("qzxsplitahead"));

        QVERIFY2(waitForServerToReceive("qzxsplitpass"), qPrintable(qsl("the game never received the password - it got: %1").arg(QString::fromUtf8(mpServer->received()))));
        QVERIFY2(!mpServer->received().contains("qzxsplitahead"), qPrintable(qsl("the command went to the game with the password - the wire holds: %1").arg(QString::fromUtf8(mpServer->received()))));
        QVERIFY2(!mpServer->received().contains("qzxspl\n") && !mpServer->received().contains("qzxsplqzx"),
                 qPrintable(qsl("the half-typed password went out on its own - the wire holds: %1").arg(QString::fromUtf8(mpServer->received()))));
    }
};

#include "CommandLineKeyHandlingTest.moc"
MUDLET_GROUPED_TEST_MAIN(CommandLineKeyHandlingTest)
