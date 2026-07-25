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
 * Systematic coverage of TTrigger's pattern matching engine - the eight
 * match_*() methods behind the trigger types offered in the editor.
 *
 * Each method is called directly instead of through the trigger engine, so a
 * failure names one pattern kind rather than the whole pipeline. Every test
 * trigger carries a probe script that records whether it ran and which capture
 * groups reached Lua; the tests assert on that, because it is the contract
 * users' scripts actually depend on, rather than on TTrigger's internals.
 *
 * A profile is required (matching consults the Lua interpreter and the console
 * buffer) but is created once for the whole class, not per test.
 *
 * Run with: ctest -R TTriggerMatchTest -V
 */

#include <QtTest/QtTest>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TTrigger.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForTTriggerMatchTest();

class TTriggerMatchTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    QList<TTrigger*> mTriggers;
    const QString mHostname = qsl("TTriggerMatch-Test");
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = qsl("localhost");

private slots: // NOLINT(readability-redundant-access-specifiers)

    void initTestCase()
    {
        initializeQRCResourcesForTTriggerMatchTest();

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
    }

    void cleanup()
    {
        qDeleteAll(mTriggers);
        mTriggers.clear();
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }


    // --- REGEX_SUBSTRING: match_substring() ------------------------------

    void test_substringMatchesAnywhereInTheLine()
    {
        auto* trigger = makeTrigger({qsl("red door")}, {REGEX_SUBSTRING});
        resetProbe();
        QVERIFY(trigger->match_substring(qsl("You see a red door here."), qsl("red door"), 0));
        QCOMPARE(fireCount(), 1);
    }

    void test_substringDoesNotMatchAbsentText()
    {
        auto* trigger = makeTrigger({qsl("blue door")}, {REGEX_SUBSTRING});
        resetProbe();
        QVERIFY(!trigger->match_substring(qsl("You see a red door here."), qsl("blue door"), 0));
        QCOMPARE(fireCount(), 0);
    }

    void test_substringIsCaseSensitive()
    {
        auto* trigger = makeTrigger({qsl("RED")}, {REGEX_SUBSTRING});
        resetProbe();
        QVERIFY(!trigger->match_substring(qsl("You see a red door."), qsl("RED"), 0));
        QCOMPARE(fireCount(), 0);
    }

    // The capture handed to Lua is the pattern that matched, not the whole line -
    // scripts index matches[1] expecting just their needle back.
    void test_substringCapturesTheNeedleNotTheLine()
    {
        auto* trigger = makeTrigger({qsl("red door")}, {REGEX_SUBSTRING});
        resetProbe();
        QVERIFY(trigger->match_substring(qsl("You see a red door here."), qsl("red door"), 0));
        QCOMPARE(captureCount(), 1);
        QCOMPARE(capture(1), qsl("red door"));
    }

    // With the /g option set every occurrence is captured, not just the first.
    void test_substringWithSlashGCapturesEveryOccurrence()
    {
        auto* trigger = makeTrigger({qsl("ha")}, {REGEX_SUBSTRING});
        trigger->mPerlSlashGOption = true;
        resetProbe();
        QVERIFY(trigger->match_substring(qsl("ha ha ha"), qsl("ha"), 0));
        QCOMPARE(captureCount(), 3);
        QCOMPARE(capture(1), qsl("ha"));
        QCOMPARE(capture(3), qsl("ha"));
    }

    void test_substringMatchesNonAsciiText()
    {
        auto* trigger = makeTrigger({QString::fromUtf8("Grüße")}, {REGEX_SUBSTRING});
        resetProbe();
        QVERIFY(trigger->match_substring(QString::fromUtf8("Der Wächter sagt: Grüße, Reisender!"), QString::fromUtf8("Grüße"), 0));
        QCOMPARE(capture(1), QString::fromUtf8("Grüße"));
    }


    // --- REGEX_PERL: match_perl() ----------------------------------------

    // matches[1] is the whole match, matches[2..] the parenthesised groups.
    void test_perlMatchesWithNumberedCaptureGroups()
    {
        auto* trigger = makeTrigger({qsl("^You see a (\\w+) (\\w+)\\.$")}, {REGEX_PERL});
        resetProbe();
        QVERIFY(matchPerl(trigger, qsl("You see a red door.")));
        QCOMPARE(captureCount(), 3);
        QCOMPARE(capture(1), qsl("You see a red door."));
        QCOMPARE(capture(2), qsl("red"));
        QCOMPARE(capture(3), qsl("door"));
    }

    void test_perlDoesNotMatchWhenPatternFails()
    {
        auto* trigger = makeTrigger({qsl("^You see a (\\w+) door\\.$")}, {REGEX_PERL});
        resetProbe();
        QVERIFY(!matchPerl(trigger, qsl("There is nothing here.")));
        QCOMPARE(fireCount(), 0);
    }

    // Named groups arrive as string keys alongside the numbered ones.
    void test_perlSuppliesNamedCaptureGroups()
    {
        auto* trigger = makeTrigger({qsl("^You see a (?<colour>\\w+) (?<object>\\w+)\\.$")}, {REGEX_PERL});
        resetProbe();
        QVERIFY(matchPerl(trigger, qsl("You see a red door.")));
        QCOMPARE(namedCapture(qsl("colour")), qsl("red"));
        QCOMPARE(namedCapture(qsl("object")), qsl("door"));
    }

    // An optional group that did not participate still occupies its slot, as an
    // empty string, so matches[] indices stay aligned with the pattern.
    void test_perlReportsUnsetOptionalGroupAsEmptyCapture()
    {
        auto* trigger = makeTrigger({qsl("^(dark )?(door)$")}, {REGEX_PERL});
        resetProbe();
        QVERIFY(matchPerl(trigger, qsl("door")));
        QCOMPARE(captureCount(), 3);
        QVERIFY2(capture(2).isEmpty(), "The unset optional group should hold its slot as an empty capture");
        QCOMPARE(capture(3), qsl("door"));
    }

    void test_perlCapturesNonAsciiText()
    {
        auto* trigger = makeTrigger({QString::fromUtf8("^Der (\\w+) sagt: (Grüße)$")}, {REGEX_PERL});
        resetProbe();
        QVERIFY(matchPerl(trigger, QString::fromUtf8("Der Wächter sagt: Grüße")));
        QCOMPARE(capture(2), QString::fromUtf8("Wächter"));
        QCOMPARE(capture(3), QString::fromUtf8("Grüße"));
    }

    void test_perlAnchorsRespectLineBoundaries()
    {
        auto* trigger = makeTrigger({qsl("^door$")}, {REGEX_PERL});
        resetProbe();
        QVERIFY(!matchPerl(trigger, qsl("a door here")));
        QCOMPARE(fireCount(), 0);
    }

    void test_perlHonoursInlineCaseInsensitiveFlag()
    {
        auto* trigger = makeTrigger({qsl("(?i)red door")}, {REGEX_PERL});
        resetProbe();
        QVERIFY(matchPerl(trigger, qsl("You see a RED DOOR here.")));
        QCOMPARE(capture(1), qsl("RED DOOR"));
    }

    // A pattern that fails to compile must never match rather than firing
    // blindly - the trigger stays inert until the user fixes the expression.
    void test_perlUncompilablePatternNeverMatches()
    {
        auto* trigger = makeTrigger({qsl("(unclosed")}, {REGEX_PERL});
        resetProbe();
        QVERIFY(!matchPerl(trigger, qsl("(unclosed")));
        QCOMPARE(fireCount(), 0);
    }


    // --- REGEX_BEGIN_OF_LINE_SUBSTRING: match_begin_of_line_substring() ---

    void test_beginOfLineMatchesAtStartOfLine()
    {
        auto* trigger = makeTrigger({qsl("You see")}, {REGEX_BEGIN_OF_LINE_SUBSTRING});
        resetProbe();
        QVERIFY(trigger->match_begin_of_line_substring(qsl("You see a red door."), qsl("You see"), 0));
        QCOMPARE(fireCount(), 1);
    }

    // This is what separates it from a plain substring trigger: the same needle
    // later in the line must not match.
    void test_beginOfLineRejectsNeedleLaterInLine()
    {
        auto* trigger = makeTrigger({qsl("red door")}, {REGEX_BEGIN_OF_LINE_SUBSTRING});
        resetProbe();
        QVERIFY(!trigger->match_begin_of_line_substring(qsl("You see a red door."), qsl("red door"), 0));
        QCOMPARE(fireCount(), 0);
    }

    void test_beginOfLineCapturesTheNeedle()
    {
        auto* trigger = makeTrigger({qsl("You see")}, {REGEX_BEGIN_OF_LINE_SUBSTRING});
        resetProbe();
        QVERIFY(trigger->match_begin_of_line_substring(qsl("You see a red door."), qsl("You see"), 0));
        QCOMPARE(captureCount(), 1);
        QCOMPARE(capture(1), qsl("You see"));
    }


    // --- REGEX_EXACT_MATCH: match_exact_match() --------------------------

    void test_exactMatchAcceptsIdenticalLine()
    {
        auto* trigger = makeTrigger({qsl("You see a red door.")}, {REGEX_EXACT_MATCH});
        resetProbe();
        QVERIFY(trigger->match_exact_match(qsl("You see a red door."), qsl("You see a red door."), 0));
        QCOMPARE(fireCount(), 1);
    }

    void test_exactMatchRejectsSubstring()
    {
        auto* trigger = makeTrigger({qsl("red door")}, {REGEX_EXACT_MATCH});
        resetProbe();
        QVERIFY(!trigger->match_exact_match(qsl("You see a red door."), qsl("red door"), 0));
        QCOMPARE(fireCount(), 0);
    }

    // Lines arrive from the buffer with their newline attached; exactly one
    // trailing newline is stripped before comparison so users do not have to
    // account for it in the pattern.
    void test_exactMatchIgnoresSingleTrailingNewline()
    {
        auto* trigger = makeTrigger({qsl("You see a red door.")}, {REGEX_EXACT_MATCH});
        resetProbe();
        QVERIFY(trigger->match_exact_match(qsl("You see a red door.\n"), qsl("You see a red door."), 0));
        QCOMPARE(fireCount(), 1);
    }

    // Only the newline is forgiven - other trailing whitespace still counts.
    void test_exactMatchRejectsTrailingWhitespace()
    {
        auto* trigger = makeTrigger({qsl("You see a red door.")}, {REGEX_EXACT_MATCH});
        resetProbe();
        QVERIFY(!trigger->match_exact_match(qsl("You see a red door. "), qsl("You see a red door."), 0));
        QCOMPARE(fireCount(), 0);
    }

    void test_exactMatchCapturesTheWholeLine()
    {
        auto* trigger = makeTrigger({qsl("You see a red door.")}, {REGEX_EXACT_MATCH});
        resetProbe();
        QVERIFY(trigger->match_exact_match(qsl("You see a red door."), qsl("You see a red door."), 0));
        QCOMPARE(captureCount(), 1);
        QCOMPARE(capture(1), qsl("You see a red door."));
    }


    // --- REGEX_LUA_CODE: match_lua_code() --------------------------------

    void test_luaConditionFiresWhenItReturnsTrue()
    {
        auto* trigger = makeTrigger({qsl("return true")}, {REGEX_LUA_CODE});
        resetProbe();
        QVERIFY(trigger->match_lua_code(0));
        QCOMPARE(fireCount(), 1);
    }

    void test_luaConditionDoesNotFireWhenItReturnsFalse()
    {
        auto* trigger = makeTrigger({qsl("return false")}, {REGEX_LUA_CODE});
        resetProbe();
        QVERIFY(!trigger->match_lua_code(0));
        QCOMPARE(fireCount(), 0);
    }

    void test_luaConditionWithUnknownPatternNumberReturnsFalse()
    {
        auto* trigger = makeTrigger({qsl("return true")}, {REGEX_LUA_CODE});
        resetProbe();
        QVERIFY(!trigger->match_lua_code(7));
        QCOMPARE(fireCount(), 0);
    }

    // A condition that fails to compile is never registered, so it can never
    // match - the same inert-until-fixed guarantee as an invalid regex.
    void test_luaConditionThatFailsToCompileNeverMatches()
    {
        auto* trigger = makeTrigger({qsl("this is not lua ===")}, {REGEX_LUA_CODE});
        resetProbe();
        QVERIFY(!trigger->match_lua_code(0));
        QCOMPARE(fireCount(), 0);
    }


    // --- REGEX_LINE_SPACER: match_line_spacer() --------------------------

    // Line spacers only mean something between conditions of a multiline
    // trigger; on their own they are a no-op that reports success so they never
    // block the rest of the chain.
    void test_lineSpacerAlwaysMatchesOutsideMultilineTriggers()
    {
        auto* trigger = makeTrigger({qsl("2")}, {REGEX_LINE_SPACER});
        resetProbe();
        QVERIFY(trigger->match_line_spacer(0));
        QCOMPARE(fireCount(), 0);
    }


    // --- REGEX_COLOR_PATTERN: match_color_pattern() ----------------------

    // Child triggers of a filter are matched with line -1 (no buffer line of
    // their own), which a colour pattern cannot work with.
    void test_colorPatternRejectsUnknownLineNumber()
    {
        auto* trigger = makeColourTrigger(scmAnsiRed, TTrigger::scmIgnored);
        resetProbe();
        QVERIFY(!trigger->match_color_pattern(-1, 0));
        QCOMPARE(fireCount(), 0);
    }

    void test_colorPatternRejectsOutOfRangePatternNumber()
    {
        auto* trigger = makeColourTrigger(scmAnsiRed, TTrigger::scmIgnored);
        resetProbe();
        QVERIFY(!trigger->match_color_pattern(0, 7));
        QCOMPARE(fireCount(), 0);
    }

    void test_colorPatternMatchesForegroundColouredText()
    {
        const int line = feedColouredLine(qsl("31"), qsl("crimson banner"));
        QVERIFY2(line >= 0, "Coloured line was not found in the console buffer");

        auto* trigger = makeColourTrigger(scmAnsiRed, TTrigger::scmIgnored);
        resetProbe();
        QVERIFY(trigger->match_color_pattern(line, 0));
        QCOMPARE(fireCount(), 1);
        QCOMPARE(capture(1), qsl("crimson banner"));
    }

    void test_colorPatternIgnoresDifferentlyColouredText()
    {
        const int line = feedColouredLine(qsl("32"), qsl("emerald banner"));
        QVERIFY2(line >= 0, "Coloured line was not found in the console buffer");

        auto* trigger = makeColourTrigger(scmAnsiRed, TTrigger::scmIgnored);
        resetProbe();
        QVERIFY(!trigger->match_color_pattern(line, 0));
        QCOMPARE(fireCount(), 0);
    }


    // --- REGEX_PROMPT: match_prompt() ------------------------------------

    void test_promptMatchesOnAPromptLine()
    {
        auto* trigger = makeTrigger({QString()}, {REGEX_PROMPT});
        const bool wasPromptLine = mpHost->mpConsole->mIsPromptLine;
        mpHost->mpConsole->mIsPromptLine = true;
        resetProbe();
        const bool matched = trigger->match_prompt(0);
        mpHost->mpConsole->mIsPromptLine = wasPromptLine;

        QVERIFY(matched);
        QCOMPARE(fireCount(), 1);
    }

    void test_promptDoesNotMatchAnOrdinaryLine()
    {
        auto* trigger = makeTrigger({QString()}, {REGEX_PROMPT});
        const bool wasPromptLine = mpHost->mpConsole->mIsPromptLine;
        mpHost->mpConsole->mIsPromptLine = false;
        resetProbe();
        const bool matched = trigger->match_prompt(0);
        mpHost->mpConsole->mIsPromptLine = wasPromptLine;

        QVERIFY(!matched);
        QCOMPARE(fireCount(), 0);
    }


    // --- multiline (AND) triggers ----------------------------------------

    // In a multiline trigger a matching condition records state instead of
    // running the script; the script only runs once every condition has been
    // satisfied, which the trigger engine decides - not match_*().
    void test_multilineConditionDefersExecution()
    {
        auto* trigger = makeTrigger({qsl("first"), qsl("second")}, {REGEX_SUBSTRING, REGEX_SUBSTRING}, true);
        resetProbe();
        QVERIFY(trigger->match_substring(qsl("the first line"), qsl("first"), 0));
        QCOMPARE(fireCount(), 0);
    }

private:
    // ANSI 1 is dark red, selected by the SGR sequence ESC[31m.
    static constexpr int scmAnsiRed = 1;

    // Recorded by every test trigger when its script runs: how many times it
    // fired, and the capture groups Lua actually received. The count is taken
    // here rather than in C++ so the tests assert on the table exactly as a
    // user's script would see it.
    static QString probeScript()
    {
        return qsl("probeFired = (probeFired or 0) + 1\n"
                   "probeMatches = {}\n"
                   "probeCaptureCount = 0\n"
                   "if matches then\n"
                   "  for k, v in pairs(matches) do probeMatches[k] = v end\n"
                   "  probeCaptureCount = #matches\n"
                   "end");
    }

    TTrigger* makeTrigger(const QStringList& patterns, const QList<int>& patternKinds, const bool isMultiline = false)
    {
        auto* trigger = new TTrigger(qsl("TTriggerMatchTest probe"), patterns, patternKinds, isMultiline, mpHost);
        trigger->setScript(probeScript());
        mTriggers.append(trigger);
        return trigger;
    }

    // Colour triggers carry their colours encoded in the pattern text, which
    // setRegexCodeList() decodes back into the colour table matching works from.
    TTrigger* makeColourTrigger(const int ansiFg, const int ansiBg) { return makeTrigger({TTrigger::createColorPatternText(ansiFg, ansiBg)}, {REGEX_COLOR_PATTERN}); }

    void resetProbe() { mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("probeFired = 0 probeCaptureCount = 0 probeMatches = {} matches = {}")); }

    bool matchPerl(TTrigger* trigger, const QString& line, const int patternNumber = 0, const int posOffset = 0)
    {
        // match_perl() wants a mutable char* of the UTF-8 encoded line, kept
        // alive for the duration of the call.
        QByteArray encoded = line.toUtf8();
        return trigger->match_perl(encoded.data(), line, patternNumber, posOffset);
    }

    lua_State* luaState() { return mpHost->getLuaInterpreter()->getLuaGlobalState(); }

    int luaNumber(const char* globalName)
    {
        lua_State* L = luaState();
        lua_getglobal(L, globalName);
        const int value = lua_isnumber(L, -1) ? static_cast<int>(lua_tonumber(L, -1)) : 0;
        lua_pop(L, 1);
        return value;
    }

    int fireCount() { return luaNumber("probeFired"); }

    int captureCount() { return luaNumber("probeCaptureCount"); }

    // Indexed as Lua sees it: capture(1) is matches[1], the whole match.
    QString capture(const int index)
    {
        lua_State* L = luaState();
        lua_getglobal(L, "probeMatches");
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            return {};
        }
        lua_rawgeti(L, -1, index);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 2);
        return value;
    }

    QString namedCapture(const QString& name)
    {
        lua_State* L = luaState();
        lua_getglobal(L, "probeMatches");
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            return {};
        }
        lua_getfield(L, -1, name.toUtf8().constData());
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 2);
        return value;
    }

    // Pushes a line carrying an SGR colour through the real display pipeline so
    // the buffer holds genuinely coloured TChars, then reports its line number.
    int feedColouredLine(const QString& sgrParameter, const QString& text)
    {
        mpHost->getLuaInterpreter()->compileAndExecuteScript(qsl("feedTriggers('\\27[%1m%2\\27[0m\\n')").arg(sgrParameter, text));
        return findBufferLine(text);
    }

    int findBufferLine(const QString& needle)
    {
        auto console = mpHost->mpConsole;
        for (int i = console->buffer.getLastLineNumber(); i >= 0; --i) {
            if (console->buffer.line(i).contains(needle)) {
                return i;
            }
        }
        return -1;
    }

    // Starts a profile the way a user would via the GUI (mirrors the helper in
    // TFeedTriggersRecursionTest).
    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        QTimer::singleShot(0, qApp, [hostname, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), hostname);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), address);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(5000)) {
            QFAIL("Profile took too long to load.");
        }
        auto* host = mudlet::self()->getActiveHost();
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy connectedSpy(&(host->mTelnet), &cTelnet::signal_connected);
        if (!connectedSpy.wait(2000)) {
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

void initializeQRCResourcesForTTriggerMatchTest()
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

#include "TTriggerMatchTest.moc"
QTEST_MAIN(TTriggerMatchTest)
