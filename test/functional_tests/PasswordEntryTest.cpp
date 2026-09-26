/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vperetokin@hey.com            *
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

// The box the game's request for hidden input (IAC WILL ECHO) is answered
// with, driven the way a player drives it: keys into the main window, the
// telnet parser fed through the loopback, and the wire read back from a
// recording server. The policy behind it is covered on its own in
// PasswordEntryPolicyTest; this file is about what the box does to the
// keyboard, the focus, the command line and the wire.
//
// Every case's comment names the line whose revert turns it red, or says
// "belt and brace" where no single line does.
//
// Budget: cTelnet::checkEchoAnomalyPattern() counts every WILL and every WONT
// ECHO that cTelnet acts on, and 5 toggles inside a 5 second window latch an
// anomaly that makes the process refuse ECHO for good. init() clears that
// window before every case, and each case may make at most 4 toggles of its
// own; the WONT cleanup() sends comes after init() has cleared the window
// again, so it is free. A repeated WILL while ECHO is up is not acted on and so
// not counted; a WONT while it is down is counted once any WILL has been seen
// on the connection.

#include <QClipboard>
#include <QContextMenuEvent>
#include <QFileInfo>
#include <QMenu>
#include <QMouseEvent>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QWindow>
#include <QtTest/QtTest>

#include <chrono>

#include "Host.h"
#include "KeyUnit.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "ProfileTestHelper.h"
#include "RecordingTelnetServer.h"
#include "TCommandLine.h"
#include "TLabel.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TPasswordEntry.h"
#include "TTextEdit.h"
#include "TUiTour.h"
#include "ctelnet.h"
#include "mudlet.h"
#include "utils.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class PasswordEntryTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    RecordingTelnetServer* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Test-Password-Entry");
    const QString mLocalhost = qsl("localhost");
    QStringList mPermKeyNames;
    QStringList mSubCommandLineNames;
    int mSubCommandLineCounter = 0;

    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    TCommandLine* commandLine() const { return mpHost->mpConsole->mpCommandLine; }
    TPasswordEntry* box() const { return mpHost->mpConsole->passwordEntry(); }
    QWindow* window() const { return mudlet::self()->windowHandle(); }
    QWidget* focusWidget() const { return mudlet::self()->focusWidget(); }

    // Feeds bytes as if the server had sent them, through the loopback so that
    // the whole sequence is processed before the call returns.
    void serverSends(QByteArray data) { mpHost->mTelnet.loopbackTest(data); }
    void serverSaysOption(const char command, const char option)
    {
        QByteArray data;
        data.append(TN_IAC).append(command).append(option);
        serverSends(data);
    }
    void serverSaysEcho(const char command) { serverSaysOption(command, OPT_ECHO); }
    bool echoNegotiatedByServer() const { return mpHost->mTelnet.hisOptionState.test(static_cast<size_t>(OPT_ECHO)); }

    // Keys into the window, which go to whatever has the keyboard focus - the
    // only way to prove the focus proxy delivers. One character at a time: there
    // is no keyClicks() for a window.
    void typeIntoWindow(const QString& text)
    {
        for (const QChar c : text) {
            QTest::keyClick(window(), c.toLatin1());
        }
    }
    void pressInWindow(const Qt::Key key, const Qt::KeyboardModifiers modifiers = Qt::NoModifier) { QTest::keyClick(window(), key, modifiers); }

    // Keys straight to a widget, for the cases whose subject is not the focus.
    static void type(QWidget* pWidget, const QString& text) { QTest::keyClicks(pWidget, text); }
    static void press(QWidget* pWidget, const Qt::Key key, const Qt::KeyboardModifiers modifiers = Qt::NoModifier)
    {
        Qt::KeyboardModifiers sentModifiers = modifiers;
#if defined(Q_OS_MACOS)
        if (key == Qt::Key_Up || key == Qt::Key_Down) {
            sentModifiers |= Qt::KeypadModifier;
        }
#endif
        QTest::keyClick(pWidget, key, sentModifiers);
    }

    // A command sent from the command line the way a player sends one.
    void playerSendsFromCommandLine(const QString& text)
    {
        commandLine()->clear();
        type(commandLine(), text);
        press(commandLine(), Qt::Key_Return);
    }

    bool runLua(const QString& script) { return mpHost->mLuaInterpreter.compileAndExecuteScript(script); }

    QString luaGlobal(const char* name) const
    {
        lua_State* L = mpHost->mLuaInterpreter.getLuaGlobalState();
        lua_getglobal(L, name);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
    }

    QByteArray asSent(const QStringList& lines) const
    {
        const QByteArray eol = mpHost->mUSE_UNIX_EOL ? QByteArrayLiteral("\n") : QByteArrayLiteral("\r\n");
        QByteArray wire;
        for (const QString& line : lines) {
            wire += line.toUtf8() + eol;
        }
        return wire;
    }

    // What the server received minus the telnet negotiation Mudlet answers a
    // WILL or WONT with, so that lines either side of a prompt read contiguous.
    QByteArray wireText() const
    {
        const QByteArray raw = mpServer->received();
        QByteArray text;
        for (int i = 0; i < raw.size(); ++i) {
            const auto byte = static_cast<unsigned char>(raw.at(i));
            if (byte != 0xff) {
                text.append(raw.at(i));
                continue;
            }
            if (i + 1 >= raw.size()) {
                break;
            }
            const auto next = static_cast<unsigned char>(raw.at(i + 1));
            if (next == 0xff) {
                text.append(raw.at(i));
                ++i;
            } else if (next == 0xfa) {
                // subnegotiation: skip to IAC SE
                const int end = raw.indexOf(QByteArrayLiteral("\xff\xf0"), i + 2);
                i = end < 0 ? raw.size() : end + 1;
            } else if (next >= 0xfb && next <= 0xfe) {
                i += 2;
            } else {
                i += 1;
            }
        }
        return text;
    }

    bool waitForServerToReceive(const QByteArray& text) const
    {
        return QTest::qWaitFor(
                [this, &text]() {
                    return wireText().contains(text);
                },
                5000);
    }

    // A sub command line of this test's own, deleted again by cleanup().
    TCommandLine* freshSubCommandLine()
    {
        const QString name = qsl("passwordEntrySubLine%1").arg(++mSubCommandLineCounter);
        auto [created, message] = mpHost->mpConsole->createCommandLine(QString(), name, 0, 0, 300, 30);
        if (!created) {
            qWarning() << "PasswordEntryTest - could not create a command line:" << message;
            return nullptr;
        }
        mSubCommandLineNames << name;
        TCommandLine* pCommandLine = mpHost->mpConsole->subCommandLineWidget(name);
        if (pCommandLine) {
            pCommandLine->mSaveCommands = false;
        }
        return pCommandLine;
    }

    void switchOffAfterwards(const QStringList& names) { mPermKeyNames << names; }

    void runDeferredDeletes() { QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); }

    // Host::setFocusOnHostActiveCommandLine() leaves 10 ms and 50 ms focus
    // retries behind, which must not fire inside a later case that has moved
    // the focus elsewhere on purpose. A plain qWait(60) does not guarantee that:
    // it can end in its sleep with the 50 ms one still pending, its last event
    // pass having come before the timer was due (seen on the macOS runner). So:
    // past both due times, then one more event pass.
    static void drainFocusRetries()
    {
        QTest::qWait(70);
        QTest::qWait(0);
    }

    void resetState()
    {
        auto& telnet = mpHost->mTelnet;
        telnet.mEchoToggleCount = 0;
        telnet.mEchoAnomalyDetected = false;
        telnet.mEchoToggleTimer.invalidate();
        for (QTimer* timer : {telnet.mTimerPasswordModeTimeout, telnet.mTimerCharacterModeDetect, telnet.mTimerLogin, telnet.mTimerPass}) {
            if (timer) {
                timer->stop();
            }
        }
        telnet.mServerRequestedSGA = false;
        telnet.mCharacterModeDetected = false;
        telnet.mAutoLoginPasswordOutstanding = false;
        mpHost->setLogin(QString());
        mpHost->setPass(QString());
        mpHost->mSecuredPasswordPending = false;
        mpHost->setDisablePasswordMasking(false);
        telnet.setAutoLoginPending(false);
        // A leftover keyboard grab eats every later key. Only when it is on:
        // TConsole::setCaretMode(false) asserts the pane's proxy is gone, which
        // only turning caret mode on does
        if (mpHost->caretEnabled()) {
            mpHost->setCaretEnabled(false);
        }
        mpHost->mCaretShortcut = Host::CaretShortcut::None;
        // Through the negotiation, never setRemoteEchoingActive(false) alone,
        // which would leave cTelnet believing ECHO is still on and the next
        // case's WILL ignored as a repeat. The Host flags are cleared anyway.
        if (mpHost->isRemoteEchoingActive() || echoNegotiatedByServer()) {
            serverSaysEcho(TN_WONT);
        }
        mpHost->setRemoteEchoingActive(false);
        mpHost->mAllowToSendCommand = true;
        mpHost->mAutoClearCommandLineAfterSend = false;
        runDeferredDeletes();
        commandLine()->clear();
        commandLine()->setFocus();
        // A pane selection left by a case would be what Ctrl+C copies in the next
        for (TTextEdit* pPane : {mpHost->mpConsole->mUpperPane, mpHost->mpConsole->mLowerPane}) {
            pPane->mSelectedRegion = QRegion();
        }
        mpServer->forgetReceived();
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

        mpServer = new RecordingTelnetServer(qApp);
        QVERIFY2(mpServer->start(), "RecordingTelnetServer failed to bind a loopback port");

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        // Written before init(): a config dir of this test's own reads as a brand
        // new installation, and the first-run tour's application-wide event
        // filter would otherwise swallow every key aimed at the main window
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

        // Focus is only ever given to a widget in the active window, and under
        // the offscreen platform nothing is shown or active until asked
        mudlet::self()->show();
        mudlet::self()->activateWindow();
        QVERIFY2(QTest::qWaitForWindowActive(mudlet::self()), "the main window never became active");
        QVERIFY(window());
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            QDir(MudletApp::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        QVERIFY(mpHost);
        resetState();
        QVERIFY2(!box(), "a box was still up when this case started");
        QTRY_VERIFY2(commandLine()->hasFocus(), "the command line did not take the keyboard focus back before this case");
    }

    void cleanup()
    {
        for (const QString& name : mSubCommandLineNames) {
            mpHost->mpConsole->deleteCommandLine(name);
        }
        mSubCommandLineNames.clear();
        for (const QString& name : mPermKeyNames) {
            mpHost->getKeyUnit()->disableKey(name);
        }
        mPermKeyNames.clear();
        runLua(qsl("if passwordEntryHandlerId then killAnonymousEventHandler(passwordEntryHandlerId) passwordEntryHandlerId = nil end\n"
                   "if passwordEntryAliasId then killAlias(passwordEntryAliasId) passwordEntryAliasId = nil end\n"));
        resetState();
    }

    // Red without TMainConsole::openPasswordEntry() creating the box, or with the
    // typed-ahead move made unconditional.
    void test_willEchoOpensABoxOverTheCommandLine()
    {
        // The command that was just sent, left selected by auto-clear off: not
        // the player's typing in progress
        commandLine()->setPlainText(qsl("look"));
        commandLine()->selectAll();

        serverSaysEcho(TN_WILL);

        QVERIFY2(box(), "WILL ECHO opened no box");
        QVERIFY(box()->isVisible());
        QCOMPARE(box()->geometry(), commandLine()->geometry());
        QCOMPARE(box()->echoMode(), QLineEdit::Password);
        QCOMPARE(box()->text(), QString());
        // The placeholder carries the Esc hint, so it has to be drawn in the
        // command line's text colour and not in the default palette's, which is
        // invisible on a dark command line
        QCOMPARE(box()->palette().color(QPalette::PlaceholderText).rgb(), commandLine()->mRegularPalette.color(QPalette::Text).rgb());
        QVERIFY(box()->palette().color(QPalette::PlaceholderText).alpha() < 255);
        QCOMPARE(commandLine()->toPlainText(), qsl("look"));
        QCOMPARE(commandLine()->textCursor().selectedText(), qsl("look"));
        QCOMPARE(commandLine()->focusProxy(), box());
        QTRY_COMPARE(focusWidget(), box());
    }

    // Red without the typed-ahead move in openPasswordEntry(), or with
    // TCommandLine::playerTypedLine() always true.
    void test_textBeingTypedWhenThePromptArrivesMovesIntoTheBox()
    {
        typeIntoWindow(qsl("hunt"));
        QCOMPARE(commandLine()->toPlainText(), qsl("hunt"));
        QVERIFY(commandLine()->playerTypedLine());

        serverSaysEcho(TN_WILL);

        QVERIFY(box());
        QCOMPARE(box()->text(), qsl("hunt"));
        QCOMPARE(commandLine()->toPlainText(), QString());
        QTRY_COMPARE(focusWidget(), box());
        typeIntoWindow(qsl("er2"));
        pressInWindow(Qt::Key_Return);
        QVERIFY2(waitForServerToReceive(asSent({qsl("hunter2")})), "the password was split between the command line and the box");

        serverSaysEcho(TN_WONT);
        QTRY_VERIFY(!box());

        // A command recalled from the history is not the player's typing, so it
        // stays where it is
        mpHost->mAutoClearCommandLineAfterSend = true;
        playerSendsFromCommandLine(qsl("north"));
        QVERIFY(waitForServerToReceive(asSent({qsl("north")})));
        QCOMPARE(commandLine()->toPlainText(), QString());
        press(commandLine(), Qt::Key_Up);
        QCOMPARE(commandLine()->toPlainText(), qsl("north"));
        QVERIFY(!commandLine()->playerTypedLine());

        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QCOMPARE(box()->text(), QString());
        QCOMPARE(commandLine()->toPlainText(), qsl("north"));
    }

    // A paste is the player's typing too. Red without
    // TCommandLine::insertFromMimeData().
    void test_textPastedBeforeThePromptArrivesMovesIntoTheBox()
    {
        QGuiApplication::clipboard()->setText(qsl("pasted2"));
        press(commandLine(), Qt::Key_V, Qt::ControlModifier);
        QCOMPARE(commandLine()->toPlainText(), qsl("pasted2"));
        QVERIFY(commandLine()->playerTypedLine());
        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QCOMPARE(box()->text(), qsl("pasted2"));
        QCOMPARE(commandLine()->toPlainText(), QString());
    }

    // Belt and brace: no single line, since the box's path skips every hook by
    // never running the code the hooks live in.
    void test_returnSendsStraightToTheWireAndNothingElseSeesIt()
    {
        mpHost->mAutoClearCommandLineAfterSend = true;
        playerSendsFromCommandLine(qsl("prior"));
        QVERIFY(waitForServerToReceive(asSent({qsl("prior")})));
        press(commandLine(), Qt::Key_Escape);
        commandLine()->setPlainText(qsl("look"));
        QVERIFY(runLua(qsl("sawSendRequest = ''\n"
                           "aliasFired = ''\n"
                           "command = 'sentinel'\n"
                           "passwordEntryHandlerId = registerAnonymousEventHandler('sysDataSendRequest', function(_, cmd) sawSendRequest = cmd end)\n"
                           "passwordEntryAliasId = tempAlias('^hunter2$', [[aliasFired = 'yes']])\n")));
        mpServer->forgetReceived();

        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QTRY_COMPARE(focusWidget(), box());
        typeIntoWindow(qsl("hunter2"));
        QCOMPARE(commandLine()->toPlainText(), qsl("look"));
        pressInWindow(Qt::Key_Return);

        QVERIFY(waitForServerToReceive(asSent({qsl("hunter2")})));
        QCOMPARE(wireText(), asSent({qsl("hunter2")}));
        QCOMPARE(commandLine()->toPlainText(), qsl("look"));
        QCOMPARE(luaGlobal("sawSendRequest"), QString());
        QCOMPARE(luaGlobal("aliasFired"), QString());
        QCOMPARE(luaGlobal("command"), qsl("sentinel"));

        serverSaysEcho(TN_WONT);
        QTRY_VERIFY(!box());
        QTRY_COMPARE(focusWidget(), commandLine());
        commandLine()->clear();
        press(commandLine(), Qt::Key_Up);
        QVERIFY2(commandLine()->toPlainText() != qsl("hunter2"), "the password typed into the box was in the command line's history");
        QCOMPARE(commandLine()->toPlainText(), qsl("prior"));
    }

    // Red with closePasswordEntry() called from the box's submit.
    void test_boxStaysUpAfterReturnUntilTheGameReleasesEcho()
    {
        serverSaysEcho(TN_WILL);
        const QPointer<TPasswordEntry> pBox = box();
        QVERIFY(pBox);
        const QString freshPlaceholder = pBox->placeholderText();
        QTRY_COMPARE(focusWidget(), pBox.data());

        typeIntoWindow(qsl("pw"));
        pressInWindow(Qt::Key_Return);
        QVERIFY(waitForServerToReceive(asSent({qsl("pw")})));

        QVERIFY2(pBox && box() == pBox, "Return closed the box, so a rejected password's retry would be typed in the clear");
        QVERIFY(pBox->isVisible());
        QCOMPARE(pBox->text(), QString());
        QVERIFY2(pBox->placeholderText() != freshPlaceholder, "the box did not say the text was sent");
        QCOMPARE(focusWidget(), pBox.data());

        serverSaysEcho(TN_WONT);
        runDeferredDeletes();
        QVERIFY2(pBox.isNull(), "WONT ECHO did not delete the box");
        QVERIFY(!box());
        QCOMPARE(commandLine()->focusProxy(), nullptr);
        QTRY_COMPARE(focusWidget(), commandLine());
    }

    // Red with closePasswordEntry() submitting, or restoring, the box's text.
    void test_wontEchoDiscardsWhatIsInTheBox()
    {
        commandLine()->setPlainText(qsl("look"));
        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QTRY_COMPARE(focusWidget(), box());
        typeIntoWindow(qsl("half"));
        QCOMPARE(box()->text(), qsl("half"));
        mpServer->forgetReceived();

        serverSaysEcho(TN_WONT);
        QTRY_VERIFY(!box());
        QTest::qWait(100);
        QCOMPARE(wireText(), QByteArray());
        QCOMPARE(commandLine()->toPlainText(), qsl("look"));
    }

    // Red without the Esc branches of TPasswordEntry::handleKeyPress(), without
    // Host::dismissPasswordEntry()'s two-step, or with
    // TCommandLine::enterCommand() not reporting the player's line.
    void test_escEmptiesThenStepsPastThenStops()
    {
        QVERIFY(runLua(qsl("passwordEntryAliasId = tempAlias('^pw$', [[send('frombox')]])")));
        serverSaysEcho(TN_WILL);
        QPointer<TPasswordEntry> pFirst = box();
        QVERIFY(pFirst);
        QTRY_COMPARE(focusWidget(), pFirst.data());

        typeIntoWindow(qsl("abc"));
        pressInWindow(Qt::Key_Escape);
        QVERIFY2(pFirst && box() == pFirst, "Esc with text in the box closed it instead of emptying it");
        QCOMPARE(pFirst->text(), QString());

        pressInWindow(Qt::Key_Escape);
        runDeferredDeletes();
        QVERIFY2(pFirst.isNull(), "Esc on an empty box did not close it");
        QVERIFY(!mpHost->passwordEntryWanted());
        QTRY_COMPARE(focusWidget(), commandLine());

        // A script's send is not the player's line, whatever the game says back
        QVERIFY(runLua(qsl("send('fromscript')")));
        QVERIFY(waitForServerToReceive(asSent({qsl("fromscript")})));
        serverSends(QByteArrayLiteral("Huh?\r\n"));
        QVERIFY2(!box(), "a script's send() and the game's reply brought the box back");

        // The player's line goes the ordinary way - the alias expands although
        // ECHO is held - and once the game answers while still hiding input the
        // box comes back, saying it is still hidden
        typeIntoWindow(qsl("pw"));
        pressInWindow(Qt::Key_Return);
        QVERIFY(waitForServerToReceive(asSent({qsl("frombox")})));
        QVERIFY2(!box(), "the box came back before the game had answered the player's line");
        serverSends(QByteArrayLiteral("Invalid password.\r\nPassword: "));
        QVERIFY2(box(), "the game's answer did not bring the box back");
        QPointer<TPasswordEntry> pSecond = box();
        QVERIFY(pSecond != pFirst);
        QVERIFY2(pSecond->placeholderText().contains(qsl("Esc again")), "the box that came back does not say a second Esc lasts until the game says otherwise");
        QTRY_COMPARE(focusWidget(), pSecond.data());

        pressInWindow(Qt::Key_Escape);
        runDeferredDeletes();
        QVERIFY2(pSecond.isNull(), "the second Esc did not close the box");
        typeIntoWindow(qsl("pw"));
        pressInWindow(Qt::Key_Return);
        QVERIFY(waitForServerToReceive(asSent({qsl("frombox"), qsl("frombox")})));
        serverSends(QByteArrayLiteral("Password: "));
        QVERIFY2(!box(), "after the second Esc the game's answer brought the box back before it released ECHO");

        serverSaysEcho(TN_WONT);
        serverSaysEcho(TN_WILL);
        QVERIFY2(box(), "the next hold got no box");
        QVERIFY2(!box()->placeholderText().contains(qsl("Esc again")), "the next hold's box thinks it follows an Esc");
    }

    // After an Esc the player answers from the command line, and the game's
    // WONT is the answer to that: no box on the way, and a command typed ahead
    // of the WONT survives. Red with gameDataArrived() called before the read is
    // parsed rather than after. Then a re-prompt that does bring the box back
    // leaves text in the command line alone. Red with openPasswordEntry()
    // moving typed-ahead text into a box that follows an Esc.
    void test_afterAnEscTheGamesAnswerDecidesAndTypedAheadTextSurvives()
    {
        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QTRY_COMPARE(focusWidget(), box());
        pressInWindow(Qt::Key_Escape);
        runDeferredDeletes();
        QVERIFY(!box());
        QTRY_COMPARE(focusWidget(), commandLine());

        typeIntoWindow(qsl("hunter2"));
        pressInWindow(Qt::Key_Return);
        QVERIFY(waitForServerToReceive(asSent({qsl("hunter2")})));
        typeIntoWindow(qsl("look"));
        serverSends(QByteArrayLiteral("Wrong.\r\nPassword: "));
        QVERIFY2(box(), "the game's re-prompt did not bring the box back");
        QCOMPARE(box()->text(), QString());
        QCOMPARE(commandLine()->toPlainText(), qsl("look"));

        serverSaysEcho(TN_WONT);
        QTRY_VERIFY(!box());
        QCOMPARE(commandLine()->toPlainText(), qsl("look"));
        QTRY_COMPARE(focusWidget(), commandLine());

        // A fresh hold, with nothing typed ahead this time
        commandLine()->clear();
        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QTRY_COMPARE(focusWidget(), box());
        pressInWindow(Qt::Key_Escape);
        runDeferredDeletes();
        QVERIFY(!box());
        typeIntoWindow(qsl("pw2"));
        pressInWindow(Qt::Key_Return);
        QVERIFY(waitForServerToReceive(asSent({qsl("hunter2"), qsl("pw2")})));
        typeIntoWindow(qsl("next"));
        // A box opened on the way would be closed again by the same WONT, so it
        // is the policy's announcements that show one never opened
        QSignalSpy wanted(mpHost, &Host::signal_passwordEntryWantedChanged);
        serverSaysEcho(TN_WONT);
        QVERIFY(!box());
        QVERIFY2(wanted.isEmpty(), "the WONT that answered the player's line let a box open on the way");
        QCOMPARE(commandLine()->toPlainText(), qsl("next"));
    }

    // Red without the QSignalBlocker in TMainConsole::appendToCommandLine().
    void test_aScriptAppendingToMainDoesNotCancelTheAutoLogin()
    {
        mpHost->setLogin(qsl("morquin"));
        mpHost->mTelnet.mTimerLogin->start(60000ms);
        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QVERIFY(runLua(qsl("appendCmdLine('main', 'x')")));
        QCOMPARE(box()->text(), qsl("x"));
        QVERIFY2(mpHost->mTelnet.mTimerLogin->isActive(), "a script's appendCmdLine() counted as the player's edit and cancelled the auto-login");
    }

    // Red without the postMessage in cTelnet::slot_passwordMaskTimeout().
    void test_theLoginPhaseTimeoutClosesTheBoxAndSaysSo()
    {
        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QTRY_COMPARE(focusWidget(), box());
        typeIntoWindow(qsl("half"));
        const int linesBefore = mpHost->mpConsole->buffer.lineBuffer.size();

        mpHost->mTelnet.slot_passwordMaskTimeout();

        QTRY_VERIFY(!box());
        QVERIFY(!mpHost->isRemoteEchoingActive());
        QTRY_VERIFY2(mpHost->mpConsole->buffer.lineBuffer.mid(qMax(0, linesBefore - 1)).join(QChar::LineFeed).contains(qsl("stopped hiding")),
                     "the timeout closed the box and dropped its text without a word");
        QCOMPARE(commandLine()->toPlainText(), QString());
    }

    // Red with TLabel writing a prompt: link's text to the command line directly.
    void test_aLabelPromptLinkFollowsTheKeyboardIntoTheBox()
    {
        QVERIFY(mpHost->mpConsole->createLabel(QString(), qsl("passwordEntryLabel"), 0, 0, 60, 20, false));
        TLabel* pLabel = mpHost->mpConsole->labelWidget(qsl("passwordEntryLabel"));
        QVERIFY(pLabel);
        const auto tidy = qScopeGuard([this]() {
            mpHost->mpConsole->deleteLabel(qsl("passwordEntryLabel"));
        });

        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        emit pLabel->linkActivated(qsl("prompt:say hi"));
        QCOMPARE(box()->text(), qsl("say hi"));
        QCOMPARE(commandLine()->toPlainText(), QString());
    }

    // Red without TPasswordEntry::copyConsoleSelection().
    void test_ctrlCInTheBoxCopiesAnOutputPaneSelection()
    {
        // Enough text that a drag across the middle of the pane crosses some of it
        for (int i = 0; i < 80; ++i) {
            mpHost->mpConsole->print(qsl("qzx qzx qzx qzx qzx qzx qzx qzx qzx qzx qzx qzx qzx qzx qzx qzx qzx qzx qzx qzx\n"));
        }
        TTextEdit* pPane = mpHost->mpConsole->mUpperPane;
        pPane->mSelectedRegion = QRegion();
        QGuiApplication::clipboard()->setText(qsl("before"));
        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QTRY_COMPARE(focusWidget(), box());

        const auto sendMouse = [pPane](const QEvent::Type type, const Qt::MouseButton button, const Qt::MouseButtons buttons, const QPointF& pos) {
            QMouseEvent event(type, pos, pPane->mapToGlobal(pos.toPoint()), button, buttons, Qt::NoModifier);
            QApplication::sendEvent(pPane, &event);
        };
        const QPointF middle = QRectF(pPane->rect()).center();
        sendMouse(QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton, middle - QPointF(60, 0));
        sendMouse(QEvent::MouseMove, Qt::NoButton, Qt::LeftButton, middle + QPointF(60, 0));
        sendMouse(QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton, middle + QPointF(60, 0));
        if (pPane->mSelectedRegion.isEmpty()) {
            QSKIP("the drag made no selection on this platform, so there is nothing to copy");
        }
        QCOMPARE(focusWidget(), box());

        pressInWindow(Qt::Key_C, Qt::ControlModifier);
        QVERIFY2(QGuiApplication::clipboard()->text().contains(qsl("qzx")), "Ctrl+C in the box did not copy the output pane's selection");
    }

    // Red with Host::recomputePasswordEntryWanted() ignoring the preference.
    void test_thePreferenceMeansNoBoxAndAnOrdinaryHistory()
    {
        mpHost->setDisablePasswordMasking(true);
        mpHost->mAutoClearCommandLineAfterSend = true;

        serverSaysEcho(TN_WILL);
        QVERIFY2(!box(), "a box opened although the profile has hidden input switched off");
        QVERIFY(mpHost->isRemoteEchoingActive());

        typeIntoWindow(qsl("inclear"));
        pressInWindow(Qt::Key_Return);
        QVERIFY(waitForServerToReceive(asSent({qsl("inclear")})));
        press(commandLine(), Qt::Key_Up);
        QCOMPARE(commandLine()->toPlainText(), qsl("inclear"));
    }

    // What GoMud sends since its #633 (June 2026): a WONT ECHO baseline after it
    // finds CLIENT_NAME, WILL SGA and WILL EOR at connect, WILL ECHO before each
    // password step and WONT ECHO once it validates, and a re-prompt under the
    // held ECHO after a rejected password - during which the character-at-a-time
    // detector may fire a false positive. Red with recognition closing the box.
    void test_gomudLoginSequence()
    {
        serverSaysOption(TN_WILL, OPT_SUPPRESS_GO_AHEAD);
        serverSaysOption(TN_WILL, OPT_EOR);
        serverSaysEcho(TN_WONT);
        serverSends(QByteArrayLiteral("Enter your name: "));
        QVERIFY(!box());
        typeIntoWindow(qsl("morquin"));
        pressInWindow(Qt::Key_Return);
        QVERIFY(waitForServerToReceive(asSent({qsl("morquin")})));

        serverSaysEcho(TN_WILL);
        serverSends(QByteArrayLiteral("Password: "));
        const QPointer<TPasswordEntry> pBox = box();
        QVERIFY2(pBox, "the password step's WILL ECHO opened no box");
        QTRY_COMPARE(focusWidget(), pBox.data());

        typeIntoWindow(qsl("wrong"));
        pressInWindow(Qt::Key_Return);
        QVERIFY(waitForServerToReceive(asSent({qsl("morquin"), qsl("wrong")})));
        serverSends(QByteArrayLiteral("Invalid password.\r\n"));
        serverSaysEcho(TN_WILL);
        serverSends(QByteArrayLiteral("Password: "));
        QVERIFY2(pBox && box() == pBox, "the re-prompt under the held ECHO lost the box");
        const QString sentPlaceholder = pBox->placeholderText();

        QTimer* detect = mpHost->mTelnet.mTimerCharacterModeDetect;
        QVERIFY2(detect && detect->isActive(), "the password line did not arm character-at-a-time detection");
        detect->start(0ms);
        QTRY_VERIFY(mpHost->mTelnet.mCharacterModeDetected);
        QVERIFY2(pBox && box() == pBox, "recognition firing on the retry closed the box, so the retry would be typed in the clear");
        QCOMPARE(pBox->placeholderText(), sentPlaceholder);

        typeIntoWindow(qsl("right"));
        pressInWindow(Qt::Key_Return);
        QVERIFY(waitForServerToReceive(asSent({qsl("morquin"), qsl("wrong"), qsl("right")})));
        QCOMPARE(wireText(), asSent({qsl("morquin"), qsl("wrong"), qsl("right")}));

        serverSaysEcho(TN_WONT);
        runDeferredDeletes();
        QVERIFY(pBox.isNull());
        QTRY_COMPARE(focusWidget(), commandLine());
    }

    // A game that holds ECHO for the session, SGA offered: recognition fires and
    // changes nothing; the player steps past with Esc, one line, Esc again. Red
    // with recognition read anywhere in the policy.
    void test_aGameThatNeverReleasesEchoTakesTwoEscsPerHold()
    {
        serverSaysOption(TN_WILL, OPT_SUPPRESS_GO_AHEAD);
        serverSaysEcho(TN_WILL);
        QPointer<TPasswordEntry> pFirst = box();
        QVERIFY(pFirst);
        QTRY_COMPARE(focusWidget(), pFirst.data());
        typeIntoWindow(qsl("x"));
        pressInWindow(Qt::Key_Return);
        QVERIFY(waitForServerToReceive(asSent({qsl("x")})));
        QTimer* detect = mpHost->mTelnet.mTimerCharacterModeDetect;
        QVERIFY(detect && detect->isActive());
        detect->start(0ms);
        QTRY_VERIFY(mpHost->mTelnet.mCharacterModeDetected);
        QVERIFY2(pFirst && box() == pFirst, "recognition closed the box");

        pressInWindow(Qt::Key_Escape);
        runDeferredDeletes();
        QVERIFY(pFirst.isNull());
        QTRY_COMPARE(focusWidget(), commandLine());

        typeIntoWindow(qsl("look"));
        pressInWindow(Qt::Key_Return);
        QVERIFY(waitForServerToReceive(asSent({qsl("x"), qsl("look")})));
        serverSends(QByteArrayLiteral("You look around.\r\n"));
        QVERIFY(box());
        QPointer<TPasswordEntry> pSecond = box();
        QVERIFY(pSecond->placeholderText().contains(qsl("Esc again")));
        QTRY_COMPARE(focusWidget(), pSecond.data());
        pressInWindow(Qt::Key_Escape);
        runDeferredDeletes();
        QVERIFY(pSecond.isNull());

        typeIntoWindow(qsl("look"));
        pressInWindow(Qt::Key_Return);
        QVERIFY(waitForServerToReceive(asSent({qsl("x"), qsl("look"), qsl("look")})));
        serverSends(QByteArrayLiteral("You look around.\r\n"));
        QVERIFY2(!box(), "the box came back after the second Esc");

        serverSaysEcho(TN_WONT);
        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QVERIFY(!box()->placeholderText().contains(qsl("Esc again")));
    }

    // Red with Host::recomputePasswordEntryWanted() ignoring the auto-login, or
    // with Host::autoLoginPasswordSent() writing its two inputs through two
    // recomputes (the typed-ahead "look" would then move into a box for an
    // instant and be destroyed with it).
    // Once the auto-login has sent the password under the game's mask, the
    // game's answer decides: a rejected password's re-prompt under the held ECHO
    // gets a box for the retry, with the command typed ahead left in the command
    // line - a game that sends its text and its WONT in separate reads would
    // otherwise move that command into the box on the first read and drop it
    // on the second. Red with the send suppressing the box for the rest of the
    // hold, or with the typed-ahead move made on a box the prompt did not open.
    void test_autoLoginKeepsTheBoxAwayUntilTheGameAnswersAndTypedAheadTextStaysPut()
    {
        mpHost->setLogin(qsl("morquin"));
        mpHost->setPass(qsl("hunter2"));
        mpHost->mTelnet.mTimerPass->start(60000ms);
        mpHost->mTelnet.setAutoLoginPending(true);

        serverSaysEcho(TN_WILL);
        QVERIFY2(!box(), "a box opened while the auto-login still intended to send the password");
        // Typed with keys: a printCmdLine() line is not the player's and would
        // pass with the two-recompute bug
        typeIntoWindow(qsl("look"));
        QCOMPARE(commandLine()->toPlainText(), qsl("look"));
        QVERIFY(commandLine()->playerTypedLine());

        mpHost->mTelnet.mTimerPass->start(0ms);
        QVERIFY(waitForServerToReceive(asSent({qsl("hunter2")})));
        QTRY_VERIFY(!mpHost->mTelnet.autoLoginPending());
        QVERIFY2(!box(), "a box opened the moment the auto-login had answered under the game's mask");
        QCOMPARE(commandLine()->toPlainText(), qsl("look"));

        serverSends(QByteArrayLiteral("\r\nWrong password.\r\nPassword: "));
        QVERIFY2(box(), "the game rejecting the stored password under the held ECHO got no box for the retry");
        QCOMPARE(box()->text(), QString());
        QCOMPARE(commandLine()->toPlainText(), qsl("look"));
        QVERIFY2(!box()->placeholderText().contains(qsl("Esc again")), "the retry's box is worded as if the player had pressed Esc");
        QTRY_COMPARE(focusWidget(), box());
        typeIntoWindow(qsl("hunter3"));
        pressInWindow(Qt::Key_Return);
        QVERIFY2(waitForServerToReceive(asSent({qsl("hunter2"), qsl("hunter3")})), "the retry did not go out from the box");

        serverSaysEcho(TN_WONT);
        QTRY_VERIFY(!box());
        QCOMPARE(commandLine()->toPlainText(), qsl("look"));
    }

    // Red with Host::autoLoginPasswordSent() suppressing whether or not ECHO was up.
    void test_autoLoginSentWithEchoOffLeavesALaterPromptItsBox()
    {
        mpHost->setLogin(qsl("morquin"));
        mpHost->setPass(qsl("hunter2"));
        mpHost->mTelnet.setAutoLoginPending(true);
        mpHost->mTelnet.mTimerPass->start(0ms);
        QVERIFY(waitForServerToReceive(asSent({qsl("hunter2")})));
        QTRY_VERIFY(!mpHost->mTelnet.autoLoginPending());

        serverSaysEcho(TN_WILL);
        QVERIFY2(box(), "a prompt after an auto-login the game never masked got no box");
    }

    // Red without slot_send_login()'s call to Host::clearPasswordEntryDismissal().
    void test_theAutoLoginNameEndsADismissal()
    {
        mpHost->setLogin(qsl("morquin"));
        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QTRY_COMPARE(focusWidget(), box());
        pressInWindow(Qt::Key_Escape);
        runDeferredDeletes();
        QVERIFY(!box());

        mpHost->mTelnet.slot_send_login();
        QVERIFY(waitForServerToReceive(asSent({qsl("morquin")})));
        serverSends(QByteArrayLiteral("Password: "));
        QVERIFY2(box(), "the auto-login's name did not bring the box back for the password prompt");
        QVERIFY(box()->placeholderText().contains(qsl("Esc again")));
    }

    // Red without the textEdited connection in TPasswordEntry's constructor, or
    // with the typed-ahead move not counting as the player's first edit.
    void test_theFirstEditInTheBoxCancelsTheAutoLogin()
    {
        mpHost->setLogin(qsl("morquin"));
        mpHost->mTelnet.mTimerLogin->start(60000ms);
        mpHost->mTelnet.mAutoLoginPasswordOutstanding = true;
        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QTRY_COMPARE(focusWidget(), box());
        QVERIFY(mpHost->mTelnet.mTimerLogin->isActive());

        typeIntoWindow(qsl("a"));
        QVERIFY2(!mpHost->mTelnet.mTimerLogin->isActive(), "the first key typed into the box did not cancel the auto-login's name step");
        QVERIFY2(!mpHost->mTelnet.mAutoLoginPasswordOutstanding, "the first key typed into the box left a late keychain password free to be typed over it");
    }

    // Text the player typed ahead of the prompt is their first edit too - but an
    // emptied line is nobody's typing and cancels nothing. Red with the
    // typed-ahead move calling Host::passwordEntryEdited() for an empty line, or
    // not at all.
    void test_textMovedIntoTheBoxCountsAsItsFirstEditUnlessEmpty()
    {
        mpHost->setLogin(qsl("morquin"));
        mpHost->mTelnet.mTimerLogin->start(60000ms);
        typeIntoWindow(qsl("b"));
        pressInWindow(Qt::Key_Backspace);
        QCOMPARE(commandLine()->toPlainText(), QString());
        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QVERIFY2(mpHost->mTelnet.mTimerLogin->isActive(), "an empty command line was taken for the player's typing and cancelled the auto-login");
        serverSaysEcho(TN_WONT);
        QTRY_VERIFY(!box());
        typeIntoWindow(qsl("hun"));
        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QCOMPARE(box()->text(), qsl("hun"));
        QVERIFY2(!mpHost->mTelnet.mTimerLogin->isActive(), "text moved into the box from the command line did not count as the player's first edit");
    }

    // Red with the box offering every key to the bindings, or none, or
    // swallowing the arrow keys whatever their modifiers.
    void test_keyBindingsRunFromTheBoxOnlyForKeysThatDoNotType()
    {
        QVERIFY(runLua(qsl("permKey('passwordEntryF7', '', 0, %1, [[send('frombinding')]])\n"
                           "permKey('passwordEntryLetterA', '', 0, %2, [[send('fromletter')]])\n"
                           "permKey('passwordEntryAltUp', '', %3, %4, [[send('frommodifiedup')]])\n"
                           "permKey('passwordEntryPlainDown', '', 0, %5, [[send('fromplaindown')]])\n")
                               .arg(static_cast<int>(Qt::Key_F7))
                               .arg(static_cast<int>(Qt::Key_A))
                               .arg(static_cast<int>(Qt::AltModifier))
                               .arg(static_cast<int>(Qt::Key_Up))
                               .arg(static_cast<int>(Qt::Key_Down))));
        switchOffAfterwards({qsl("passwordEntryF7"), qsl("passwordEntryLetterA"), qsl("passwordEntryAltUp"), qsl("passwordEntryPlainDown")});

        serverSaysEcho(TN_WILL);
        const QPointer<TPasswordEntry> pBox = box();
        QVERIFY(pBox);
        QTRY_COMPARE(focusWidget(), pBox.data());

        pressInWindow(Qt::Key_F7);
        QVERIFY2(waitForServerToReceive(asSent({qsl("frombinding")})), "an F-key binding did not run from the box");
        QVERIFY(pBox && box() == pBox);
        QCOMPARE(pBox->text(), QString());

        typeIntoWindow(qsl("a"));
        QCOMPARE(pBox->text(), qsl("a"));
        QTest::qWait(50);
        QVERIFY2(!wireText().contains("fromletter"), "a binding on a plain letter ate a character of the password");

        // As in the command line: a plain arrow key is the history's and never a
        // binding's, one with a modifier is a binding's
        pressInWindow(Qt::Key_Up, Qt::AltModifier);
        QVERIFY2(waitForServerToReceive(asSent({qsl("frommodifiedup")})), "a binding on a modified arrow key did not run from the box");
        QCOMPARE(focusWidget(), pBox.data());
        pressInWindow(Qt::Key_Down);
        QTest::qWait(50);
        QVERIFY2(!wireText().contains("fromplaindown"), "a binding on a plain arrow key ran from the box");
        QCOMPARE(pBox->text(), qsl("a"));
    }

    // Belt and brace: two layers, no single line.
    void test_revealedTextIsStillNotForTheClipboardAndUndoDoesNotBringItBack()
    {
        QGuiApplication::clipboard()->setText(qsl("before"));
        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QTRY_COMPARE(focusWidget(), box());
        typeIntoWindow(qsl("secret"));

        QAction* pReveal = nullptr;
        for (QAction* pAction : box()->actions()) {
            if (!pAction->text().isEmpty()) {
                pReveal = pAction;
            }
        }
        QVERIFY2(pReveal, "the box has no reveal action");
        pReveal->trigger();
        QCOMPARE(box()->echoMode(), QLineEdit::Normal);
        QVERIFY(box()->inputMethodHints() & Qt::ImhHiddenText);
        box()->selectAll();
        pressInWindow(Qt::Key_C, Qt::ControlModifier);
        QCOMPARE(QGuiApplication::clipboard()->text(), qsl("before"));

        // A keyboard or mouse selection in a Normal-mode QLineEdit goes to the
        // selection clipboard on platforms that have one (red without
        // slot_selectionClipboardChanged()); the offscreen platform has none,
        // so there this half proves nothing
        if (QGuiApplication::clipboard()->supportsSelection()) {
            QGuiApplication::clipboard()->setText(qsl("primaryBefore"), QClipboard::Selection);
            box()->deselect();
            pressInWindow(Qt::Key_Home, Qt::ShiftModifier);
            QCOMPARE(box()->selectedText(), qsl("secret"));
            QVERIFY2(QGuiApplication::clipboard()->text(QClipboard::Selection) != qsl("secret"), "a keyboard selection of the revealed password went to the selection clipboard");
            QTest::mousePress(box(), Qt::LeftButton, Qt::NoModifier, QPoint(3, box()->height() / 2));
            QTest::mouseMove(box(), QPoint(box()->width() - 30, box()->height() / 2));
            QTest::mouseRelease(box(), Qt::LeftButton, Qt::NoModifier, QPoint(box()->width() - 30, box()->height() / 2));
            QVERIFY2(QGuiApplication::clipboard()->text(QClipboard::Selection) != qsl("secret"), "a mouse selection of the revealed password went to the selection clipboard");
        }
        pReveal->trigger();
        QCOMPARE(box()->echoMode(), QLineEdit::Password);

        pressInWindow(Qt::Key_Return);
        QVERIFY(waitForServerToReceive(asSent({qsl("secret")})));
        QCOMPARE(box()->text(), QString());
        pressInWindow(Qt::Key_Z, Qt::ControlModifier);
        QCOMPARE(box()->text(), QString());
    }

    // Red without the Tab/Up/Down swallow (Tab: focus would move on), or without
    // the caret shortcut branch of handleKeyPress(); the typed key coming back
    // is red without TCommandLine::event()'s redirect to its proxy.
    void test_tabStaysAndTheCaretShortcutLeavesForThePane()
    {
        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QTRY_COMPARE(focusWidget(), box());
        typeIntoWindow(qsl("ab"));
        pressInWindow(Qt::Key_Tab);
        pressInWindow(Qt::Key_Up);
        pressInWindow(Qt::Key_Down);
        // With a modifier and no binding on it, a Tab is still no way out
        pressInWindow(Qt::Key_Backtab, Qt::ShiftModifier);
        pressInWindow(Qt::Key_Tab, Qt::ControlModifier);
        pressInWindow(Qt::Key_Tab, Qt::MetaModifier);
        QCOMPARE(box()->text(), qsl("ab"));
        QCOMPARE(focusWidget(), box());

        mpHost->mCaretShortcut = Host::CaretShortcut::F6;
        pressInWindow(Qt::Key_F6);
        QTRY_VERIFY2(mpHost->caretEnabled(), "the caret shortcut did not turn caret mode on from the box");
        QTRY_COMPARE(focusWidget(), mpHost->mpConsole->mUpperPane);
        QVERIFY2(box(), "leaving for the pane closed the box");

        typeIntoWindow(qsl("c"));
        QTRY_COMPARE(box()->text(), qsl("abc"));
        QCOMPARE(commandLine()->toPlainText(), QString());
        // The forwarder went through Host::setFocusOnHostActiveCommandLine()
        drainFocusRetries();
    }

    // Red without TCommandLine::event()'s redirect to its proxy.
    void test_aSyntheticKeySentToTheCommandLineLandsInTheBox()
    {
        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, qsl("a"));
        qApp->sendEvent(commandLine(), &keyPress);
        QCOMPARE(box()->text(), qsl("a"));
        QCOMPARE(commandLine()->toPlainText(), QString());
    }

    // Red without the focus proxy (the setFocus() half), or without
    // TPasswordEntry::focusInEvent() recording the command line as the active
    // one (the setFocusOnHostActiveCommandLine() half: the sub line was the last
    // command line focused before the box, so without that record the focus
    // would go back to it).
    void test_focusMeantForTheCommandLineLandsOnTheBox()
    {
        TCommandLine* pSubLine = freshSubCommandLine();
        QVERIFY(pSubLine);
        pSubLine->setFocus();
        QTRY_COMPARE(focusWidget(), pSubLine);

        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QTRY_COMPARE(focusWidget(), box());

        pSubLine->setFocus();
        QTRY_COMPARE(focusWidget(), pSubLine);
        commandLine()->setFocus();
        QTRY_COMPARE(focusWidget(), box());

        // Somewhere that is no command line, so that nothing but the record
        // decides where "the active command line" is
        QLineEdit elsewhere(mudlet::self());
        elsewhere.show();
        elsewhere.setFocus();
        QTRY_COMPARE(focusWidget(), &elsewhere);
        mpHost->setFocusOnHostActiveCommandLine();
        QTRY_COMPARE(focusWidget(), box());
        drainFocusRetries();
    }

    // Red with openPasswordEntry() taking focus from any widget.
    void test_theBoxDoesNotStealFocusFromOutsideTheCommandLines()
    {
        // No earlier case's focus retry may decide this one
        drainFocusRetries();
        QLineEdit editor(mudlet::self());
        editor.show();
        editor.setFocus();
        QTRY_COMPARE(focusWidget(), &editor);

        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QCOMPARE(focusWidget(), &editor);
        QTest::qWait(60);
        QCOMPARE(focusWidget(), &editor);

        // Focus meant for the command line still lands on the box
        commandLine()->setFocus();
        QTRY_COMPARE(focusWidget(), box());
    }

    // Red with closePasswordEntry() focusing the command line unconditionally.
    void test_closingABoxThatDoesNotHaveFocusLeavesFocusAlone()
    {
        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QTRY_COMPARE(focusWidget(), box());
        TCommandLine* pSubLine = freshSubCommandLine();
        QVERIFY(pSubLine);
        pSubLine->setFocus();
        QTRY_COMPARE(focusWidget(), pSubLine);

        serverSaysEcho(TN_WONT);
        QTRY_VERIFY(!box());
        QCOMPARE(focusWidget(), pSubLine);
        QTest::qWait(60);
        QCOMPARE(focusWidget(), pSubLine);
    }

    // Red without the event filter in openPasswordEntry().
    void test_theBoxFollowsTheCommandLinesGeometry()
    {
        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QCOMPARE(box()->geometry(), commandLine()->geometry());
        const QSize before = mudlet::self()->size();
        const QRect commandLineBefore = commandLine()->geometry();
        mudlet::self()->resize(before + QSize(120, 60));
        QTRY_VERIFY2(commandLine()->geometry() != commandLineBefore, "resizing the window did not move the command line, so nothing below is tested");
        QTRY_COMPARE(box()->geometry(), commandLine()->geometry());
        mudlet::self()->resize(before);
        QTRY_COMPARE(box()->geometry(), commandLine()->geometry());
    }

    // Red with the isRemoteEchoingActive() refusal back in
    // TLuaInterpreter::callCmdLineAction().
    void test_aSubCommandLineActionRunsDuringAPrompt()
    {
        TCommandLine* pSubLine = freshSubCommandLine();
        QVERIFY(pSubLine);
        QVERIFY(runLua(qsl("actionSaw = ''\nsetCmdLineAction('%1', function(text) actionSaw = text end)").arg(mSubCommandLineNames.last())));

        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        pSubLine->setFocus();
        QTRY_COMPARE(focusWidget(), pSubLine);
        typeIntoWindow(qsl("hi"));
        pressInWindow(Qt::Key_Return);
        QCOMPARE(luaGlobal("actionSaw"), qsl("hi"));
        QVERIFY(box());
    }

    // Red without the box branches of TMainConsole's command-line methods.
    void test_luaWritesToMainGoIntoTheBoxAndReadsDoNot()
    {
        commandLine()->setPlainText(qsl("look"));
        serverSaysEcho(TN_WILL);
        QVERIFY(box());

        QVERIFY(runLua(qsl("printCmdLine('main', 'fromlua')")));
        QCOMPARE(box()->text(), qsl("fromlua"));
        QCOMPARE(commandLine()->toPlainText(), qsl("look"));
        QVERIFY(runLua(qsl("readBack = getCmdLine('main')")));
        QCOMPARE(luaGlobal("readBack"), qsl("look"));

        QVERIFY(runLua(qsl("sendCmdLine('fromlink')")));
        QCOMPARE(box()->text(), qsl("fromlink"));
        QCOMPARE(box()->selectedText(), qsl("fromlink"));
        QVERIFY(runLua(qsl("appendCmdLine('main', 'x')")));
        QCOMPARE(box()->text(), qsl("fromlinkx"));
        QVERIFY(runLua(qsl("selectCmdLineText('main')")));
        QCOMPARE(box()->selectedText(), qsl("fromlinkx"));
        QVERIFY(runLua(qsl("clearCmdLine('main')")));
        QCOMPARE(box()->text(), QString());
        QCOMPARE(commandLine()->toPlainText(), qsl("look"));

        // The same writes reach the command line once the box is gone
        serverSaysEcho(TN_WONT);
        QTRY_VERIFY(!box());
        QVERIFY(runLua(qsl("printCmdLine('main', 'after')")));
        QCOMPARE(commandLine()->toPlainText(), qsl("after"));
        QVERIFY(runLua(qsl("appendCmdLine('main', '!')")));
        QCOMPARE(commandLine()->toPlainText(), qsl("after!"));
    }

    // Red without submit() stripping the carriage return (sendData() strips the
    // line feed on its own).
    void test_pasteWithATrailingLineBreakSendsOneLine()
    {
        QGuiApplication::clipboard()->setText(qsl("pw\r\n"));
        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QTRY_COMPARE(focusWidget(), box());
        pressInWindow(Qt::Key_V, Qt::ControlModifier);
        QVERIFY(box()->text().startsWith(qsl("pw")));
        mpServer->forgetReceived();
        pressInWindow(Qt::Key_Return);
        QVERIFY(waitForServerToReceive(asSent({qsl("pw")})));
        QCOMPARE(wireText(), asSent({qsl("pw")}));
    }

    // Red with QLineEdit's standard context menu.
    void test_theContextMenuOffersPasteOnly()
    {
        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        const QPoint where(5, 5);
        QContextMenuEvent menuEvent(QContextMenuEvent::Mouse, where, box()->mapToGlobal(where));
        qApp->sendEvent(box(), &menuEvent);
        QMenu* pMenu = box()->findChild<QMenu*>();
        QVERIFY2(pMenu, "no context menu was shown");
        const auto closeMenu = qScopeGuard([pMenu]() {
            pMenu->close();
        });
        QCOMPARE(pMenu->actions().size(), 1);
        QVERIFY(pMenu->actions().first()->text().contains(qsl("Paste")));
    }

    // Red with closePasswordEntry() consulting QApplication::focusWidget(),
    // which is null while Mudlet is not the active application.
    void test_closingWhileTheWindowIsInactiveLeavesFocusOnTheCommandLine()
    {
        serverSaysEcho(TN_WILL);
        QVERIFY(box());
        QTRY_COMPARE(focusWidget(), box());

        QWidget other;
        other.resize(100, 100);
        other.show();
        other.activateWindow();
        QTRY_VERIFY2(!mudlet::self()->isActiveWindow(), "the main window stayed active");

        serverSaysEcho(TN_WONT);
        QTRY_VERIFY(!box());

        other.hide();
        mudlet::self()->activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(mudlet::self()));
        QTRY_COMPARE(focusWidget(), commandLine());
    }

    // Red with submit() claiming "Sent" whatever sendPasswordEntry() returned.
    // Last, because it takes the connection down.
    void test_enterWhileNotConnectedSaysSo()
    {
        mpHost->mTelnet.disconnectIt();
        QTRY_COMPARE(mpHost->mTelnet.getConnectionState(), QAbstractSocket::UnconnectedState);
        // The disconnect's reset() runs from the socket's signal a moment later
        // and would release ECHO under a box opened before it
        QTest::qWait(200);
        mpHost->setRemoteEchoingActive(true);
        QTRY_VERIFY(box());
        QTRY_COMPARE(focusWidget(), box());
        const int linesBefore = mpHost->mpConsole->buffer.lineBuffer.size();

        typeIntoWindow(qsl("pw"));
        pressInWindow(Qt::Key_Return);

        QVERIFY(box());
        QCOMPARE(box()->text(), QString());
        QVERIFY2(box()->placeholderText().contains(qsl("Not sent")), "the box claimed the line was sent to a game it is not connected to");
        QTRY_VERIFY2(mpHost->mpConsole->buffer.lineBuffer.mid(qMax(0, linesBefore - 1)).join(QChar::LineFeed).contains(qsl("not connected")), "nothing said the line was dropped");
    }
};

#include "PasswordEntryTest.moc"
MUDLET_GROUPED_TEST_MAIN(PasswordEntryTest)
