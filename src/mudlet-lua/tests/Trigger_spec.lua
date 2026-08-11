describe("Trigger processing", function()

    -- Test for nested trigger processing with self-deletion
    -- This verifies the fix that uses mProcessingDepth counter instead of a bool flag
    -- (same fix as for aliases - see Alias_spec.lua for detailed explanation)
    describe("nested processing", function()

        it("should not crash when inner trigger kills itself during nested feedTriggers", function()
            local inner_id
            local outer_executed = false
            local inner_executed = false

            local outer_id = tempRegexTrigger("^outer_trigger_test$", function()
                outer_executed = true
                feedTriggers("\nouter_trigger_test_inner\n")
            end)

            inner_id = tempRegexTrigger("^outer_trigger_test_inner$", function()
                inner_executed = true
                killTrigger(inner_id)
            end)

            -- Verify both triggers exist before the test
            assert.are.equal(1, exists(outer_id, "trigger"), "Outer trigger should exist before test")
            assert.are.equal(1, exists(inner_id, "trigger"), "Inner trigger should exist before test")

            -- This should not crash - the fix defers cleanup until all processing completes
            feedTriggers("\nouter_trigger_test\n")

            assert.is_true(outer_executed, "Outer trigger should have executed")
            assert.is_true(inner_executed, "Inner trigger should have executed")

            -- Verify cleanup ran correctly: inner trigger should be deleted
            assert.are.equal(0, exists(inner_id, "trigger"), "Inner trigger should have been cleaned up")

            -- Verify outer trigger still exists (wasn't incorrectly deleted)
            assert.are.equal(1, exists(outer_id, "trigger"), "Outer trigger should still exist")

            -- Cleanup
            killTrigger(outer_id)
        end)

        it("should handle double-nested trigger processing with cleanup", function()
            local level3_id
            local executions = {}

            local level1_id = tempRegexTrigger("^trigger_level1$", function()
                table.insert(executions, "level1_start")
                feedTriggers("\ntrigger_level2\n")
                table.insert(executions, "level1_end")
            end)

            local level2_id = tempRegexTrigger("^trigger_level2$", function()
                table.insert(executions, "level2_start")
                feedTriggers("\ntrigger_level3\n")
                table.insert(executions, "level2_end")
            end)

            level3_id = tempRegexTrigger("^trigger_level3$", function()
                table.insert(executions, "level3")
                killTrigger(level3_id)
            end)

            -- Verify all triggers exist before the test
            assert.are.equal(1, exists(level1_id, "trigger"), "Level 1 trigger should exist")
            assert.are.equal(1, exists(level2_id, "trigger"), "Level 2 trigger should exist")
            assert.are.equal(1, exists(level3_id, "trigger"), "Level 3 trigger should exist")

            -- This tests 3 levels of nesting with cleanup at the deepest level
            feedTriggers("\ntrigger_level1\n")

            -- Verify all levels executed in correct order (depth-first)
            assert.are.equal(5, #executions, "All execution points should have been reached")
            assert.are.equal("level1_start", executions[1])
            assert.are.equal("level2_start", executions[2])
            assert.are.equal("level3", executions[3])
            assert.are.equal("level2_end", executions[4])
            assert.are.equal("level1_end", executions[5])

            -- Verify level3 was cleaned up, others still exist
            assert.are.equal(0, exists(level3_id, "trigger"), "Level 3 trigger should have been cleaned up")
            assert.are.equal(1, exists(level1_id, "trigger"), "Level 1 trigger should still exist")
            assert.are.equal(1, exists(level2_id, "trigger"), "Level 2 trigger should still exist")

            -- Cleanup
            killTrigger(level1_id)
            killTrigger(level2_id)
        end)

        it("should handle multiple triggers being killed in nested processing", function()
            local inner1_id, inner2_id
            local execution_order = {}

            local outer_id = tempRegexTrigger("^trigger_multi_outer$", function()
                table.insert(execution_order, "outer_start")
                feedTriggers("\ntrigger_multi_inner1\n")
                feedTriggers("\ntrigger_multi_inner2\n")
                table.insert(execution_order, "outer_end")
            end)

            inner1_id = tempRegexTrigger("^trigger_multi_inner1$", function()
                table.insert(execution_order, "inner1")
                killTrigger(inner1_id)
            end)

            inner2_id = tempRegexTrigger("^trigger_multi_inner2$", function()
                table.insert(execution_order, "inner2")
                killTrigger(inner2_id)
            end)

            feedTriggers("\ntrigger_multi_outer\n")

            -- Verify execution order
            assert.are.equal(4, #execution_order)
            assert.are.equal("outer_start", execution_order[1])
            assert.are.equal("inner1", execution_order[2])
            assert.are.equal("inner2", execution_order[3])
            assert.are.equal("outer_end", execution_order[4])

            -- Both inner triggers should be cleaned up
            assert.are.equal(0, exists(inner1_id, "trigger"), "Inner1 should be cleaned up")
            assert.are.equal(0, exists(inner2_id, "trigger"), "Inner2 should be cleaned up")
            assert.are.equal(1, exists(outer_id, "trigger"), "Outer should still exist")

            killTrigger(outer_id)
        end)

    end)

    -- Color triggers must match the colors a line arrived with, even when an
    -- earlier trigger in the same pass has already recolored it. The display
    -- must still show the recolored version. Recoloring uses the same
    -- TConsole::setFgColor path as the colorizer trigger checkbox, so this
    -- covers both channels.
    --
    -- The color trigger callbacks are string code because tempAnsiColorTrigger
    -- does not run function callbacks when the expiry argument is omitted, and
    -- assertions check containment because default-palette text also matches
    -- ANSI white-on-black, so the triggers can fire on unrelated lines too.
    describe("color trigger original-color matching", function()

        local function contains(list, value)
            for _, v in ipairs(list) do
                if v == value then
                    return true
                end
            end
            return false
        end

        it("should match original colors after an earlier trigger recolors the line", function()
            _G.colorSnapshotMatches = {}
            local highlighted = false
            local lineNumber = nil

            local highlightTrigger = tempRegexTrigger("^ColorSnapshotTest$", function()
                lineNumber = getLineNumber()
                if selectString("ColorSnapshotTest", 1) > -1 then
                    setFgColor(255, 0, 0)
                    setBgColor(255, 255, 0)
                    highlighted = true
                end
                resetFormat()
            end)
            -- ANSI 7 = white foreground, ANSI 0 = black background
            local colorTrigger = tempAnsiColorTrigger(7, 0,
                [[table.insert(_G.colorSnapshotMatches, matches[1])]])

            feedTriggers("\n\27[37;40mColorSnapshotTest\27[0m\n")

            local matched = contains(_G.colorSnapshotMatches, "ColorSnapshotTest")
            killTrigger(highlightTrigger)
            killTrigger(colorTrigger)
            _G.colorSnapshotMatches = nil

            assert.is_true(highlighted, "Highlighting trigger should have run")
            assert.is_true(matched, "Color trigger should match the original colors despite the recoloring")

            -- The display must keep the recolored version
            moveCursor(0, lineNumber)
            selectString("ColorSnapshotTest", 1)
            local r, g, b = getFgColor()
            deselect()
            resetFormat()
            assert.are.equal(255, r, "Display should show the recolored foreground")
            assert.are.equal(0, g)
            assert.are.equal(0, b)
        end)

        it("should match original colors when the color trigger runs before the recoloring one", function()
            _G.colorControlMatches = {}
            local highlighted = false

            local colorTrigger = tempAnsiColorTrigger(7, 0,
                [[table.insert(_G.colorControlMatches, matches[1])]])
            local highlightTrigger = tempRegexTrigger("^ColorSnapshotControl$", function()
                if selectString("ColorSnapshotControl", 1) > -1 then
                    setFgColor(255, 0, 0)
                    highlighted = true
                end
                resetFormat()
            end)

            feedTriggers("\n\27[37;40mColorSnapshotControl\27[0m\n")

            local matched = contains(_G.colorControlMatches, "ColorSnapshotControl")
            killTrigger(colorTrigger)
            killTrigger(highlightTrigger)
            _G.colorControlMatches = nil

            assert.is_true(matched, "Color trigger should match when it runs first")
            assert.is_true(highlighted, "Highlighting trigger should have run")
        end)

        it("should keep the outer line's original colors across a nested feedTriggers", function()
            _G.innerSnapshotMatches = {}
            _G.outerSnapshotMatches = {}

            local outerTrigger = tempRegexTrigger("^OuterSnapshotLine$", function()
                if selectString("OuterSnapshotLine", 1) > -1 then
                    setFgColor(0, 0, 255)
                end
                resetFormat()
                -- ANSI 32/41 = green foreground on red background
                feedTriggers("\n\27[32;41mInnerSnapshotLine\27[0m\n")
            end)
            local innerColorTrigger = tempAnsiColorTrigger(2, 1,
                [[table.insert(_G.innerSnapshotMatches, matches[1])]])
            local outerColorTrigger = tempAnsiColorTrigger(7, 0,
                [[table.insert(_G.outerSnapshotMatches, matches[1])]])

            feedTriggers("\n\27[37;40mOuterSnapshotLine\27[0m\n")

            local innerMatched = contains(_G.innerSnapshotMatches, "InnerSnapshotLine")
            local outerMatched = contains(_G.outerSnapshotMatches, "OuterSnapshotLine")
            killTrigger(outerTrigger)
            killTrigger(innerColorTrigger)
            killTrigger(outerColorTrigger)
            _G.innerSnapshotMatches = nil
            _G.outerSnapshotMatches = nil

            assert.is_true(innerMatched, "Inner pass should match the inner line's original colors")
            assert.is_true(outerMatched, "Outer pass should still match its original colors after the nested pass")
        end)

    end)

    describe("tempAnsiColorTrigger callbacks", function()

        it("should fire a function callback when the expiry argument is omitted", function()
            _G.ansiColorFunctionFired = false
            -- ANSI 7 = white foreground, ANSI 0 = black background
            local colorTrigger = tempAnsiColorTrigger(7, 0, function()
                _G.ansiColorFunctionFired = true
            end)

            feedTriggers("\n\27[37;40mAnsiColorFunctionCallback\27[0m\n")

            local fired = _G.ansiColorFunctionFired
            killTrigger(colorTrigger)
            _G.ansiColorFunctionFired = nil

            assert.is_true(fired, "Function callback without an expiry argument should fire")
        end)

        it("should match on background colour alone when the foreground is ignored", function()
            _G.ansiBackgroundOnlyFired = false
            -- -1 ignores the foreground colour, ANSI 4 is a blue background:
            -- match any foreground as long as the background is blue.
            local colorTrigger, err = tempAnsiColorTrigger(-1, 4, function()
                _G.ansiBackgroundOnlyFired = true
            end)

            assert.is_number(colorTrigger, "Ignoring the foreground with a background colour set should create a trigger, got: " .. tostring(err))

            -- Green foreground (32) on a non-matching black background (40): it
            -- must NOT fire, proving the background is genuinely discriminated
            -- rather than the trigger having ignored both colours.
            feedTriggers("\n\27[32;40mAnsiBackgroundNonMatch\27[0m\n")
            assert.is_false(_G.ansiBackgroundOnlyFired, "A background-only trigger must not fire on a non-matching background")

            -- Red foreground (31) on the matching blue background (44): the
            -- foreground differs from any fixed value yet the trigger should
            -- still match on the background alone.
            feedTriggers("\n\27[31;44mAnsiBackgroundOnlyMatch\27[0m\n")

            local fired = _G.ansiBackgroundOnlyFired
            if type(colorTrigger) == "number" then
                killTrigger(colorTrigger)
            end
            _G.ansiBackgroundOnlyFired = nil

            assert.is_true(fired, "A background-only ANSI colour trigger (ignored foreground) should fire on a matching background")
        end)

        it("should still reject ignoring the foreground when the background is omitted", function()
            -- (-1, code) omits the background, so both colours would be ignored:
            -- this must remain an error, not create a match-everything trigger.
            local colorTrigger, err = tempAnsiColorTrigger(-1, function() end)

            if type(colorTrigger) == "number" then
                killTrigger(colorTrigger)
            end

            assert.is_nil(colorTrigger, "Ignoring the foreground with the background omitted should be rejected")
            assert.is_string(err, "A rejection should return an error message")
            assert.is_truthy(err:find("ignore both foreground and background", 1, true), "The error should explain both colours cannot be ignored, got: " .. tostring(err))
        end)

    end)

    -- feedTelnet only performs injection while the telnet socket is unconnected;
    -- otherwise it refuses with nil + a message. Its successful-injection and
    -- prompt paths cannot be exercised in busted. The type-check happens before
    -- the connection check, so the argument-type contract is still verifiable.
    describe("feedTelnet contract", function()

        it("raises an error when the data argument is not a string", function()
            -- a table is never string-coercible, unlike a number
            assert.has_error(function() feedTelnet({}) end)
        end)

        it("refuses to inject while the socket is not unconnected", function()
            -- safety property: feedTelnet never injects into a live connection.
            -- Establish the precondition here rather than relying on the
            -- profile's ambient socket state, which other specs may have cleared
            -- with disconnect(): reconnect() starts a fresh lookup and leaves the
            -- unconnected state synchronously, without the connection needing to
            -- succeed. Restore a clean state afterwards with disconnect().
            reconnect()
            local ok, msg = feedTelnet("some server data")
            disconnect()
            assert.is_nil(ok, "feedTelnet must not succeed against a non-unconnected socket")
            assert.is_string(msg)
            assert.is_truthy(msg:find("refused", 1, true), "expected a refusal message, got: " .. tostring(msg))
        end)

    end)

    describe("feedTriggers contract", function()

        it("raises an error when the data argument is not a string", function()
            assert.has_error(function() feedTriggers({}) end)
        end)

    end)

    describe("temporary trigger creation and firing", function()

        after_each(function()
            _G.TrigSpec = nil
        end)

        it("tempTrigger fires on a substring match", function()
            _G.TrigSpec = {fired = false}
            local id = tempTrigger("substr_needle", function() _G.TrigSpec.fired = true end)
            assert.is_number(id)
            feedTriggers("\nhere is a substr_needle inside a line\n")
            local fired = _G.TrigSpec.fired
            killTrigger(id)
            assert.is_true(fired, "a substring trigger should fire when its pattern appears anywhere in the line")
        end)

        it("tempExactMatchTrigger fires only on an exact whole-line match", function()
            _G.TrigSpec = {count = 0}
            local id = tempExactMatchTrigger("exact_line_only", function() _G.TrigSpec.count = _G.TrigSpec.count + 1 end)
            assert.is_number(id)
            feedTriggers("\nexact_line_only and more\n")
            assert.is_equal(0, _G.TrigSpec.count, "an exact-match trigger must not fire on a superset line")
            feedTriggers("\nexact_line_only\n")
            local count = _G.TrigSpec.count
            killTrigger(id)
            assert.is_equal(1, count, "an exact-match trigger should fire on the exact line")
        end)

        it("tempBeginOfLineTrigger matches at the start of a line only", function()
            _G.TrigSpec = {count = 0}
            local id = tempBeginOfLineTrigger("bol_marker", function() _G.TrigSpec.count = _G.TrigSpec.count + 1 end)
            assert.is_number(id)
            feedTriggers("\nnot at start bol_marker\n")
            assert.is_equal(0, _G.TrigSpec.count, "a begin-of-line trigger must not fire mid-line")
            feedTriggers("\nbol_marker at the start\n")
            local count = _G.TrigSpec.count
            killTrigger(id)
            assert.is_equal(1, count, "a begin-of-line trigger should fire when the pattern starts the line")
        end)

        it("tempRegexTrigger accepts a string body and fires on a regex match", function()
            _G.TrigSpec = {fired = false}
            local id = tempRegexTrigger("^regex_str_body\\d+$", [[_G.TrigSpec.fired = true]])
            assert.is_number(id)
            feedTriggers("\nregex_str_body123\n")
            local fired = _G.TrigSpec.fired
            killTrigger(id)
            assert.is_true(fired, "a regex trigger created from a string body should fire")
        end)

        it("tempLineTrigger fires for lines inside its window", function()
            _G.TrigSpec = {count = 0}
            -- a wide window from the current line so the fed lines fall inside it
            -- regardless of the exact off-by-one of the window start
            local id = tempLineTrigger(0, 10, function() _G.TrigSpec.count = _G.TrigSpec.count + 1 end)
            assert.is_number(id)
            feedTriggers("\nline_window_one\n")
            feedTriggers("\nline_window_two\n")
            feedTriggers("\nline_window_three\n")
            local count = _G.TrigSpec.count
            -- a line trigger matches by position, not content, so disable it
            -- immediately: killTrigger's cleanup is deferred and a still-live line
            -- trigger would otherwise fire on lines fed by later specs
            disableTrigger(id)
            killTrigger(id)
            -- three content lines fed into a 10-line window, none truncated
            assert.is_true(count >= 3, "a line trigger should fire for every line inside its window, fired " .. count .. " times")
        end)

        it("tempLineTrigger requires numeric line arguments", function()
            assert.has_error(function() tempLineTrigger("start", 2, [[]]) end)
        end)

    end)

    describe("temporary trigger argument validation", function()

        it("tempTrigger rejects a non-string, non-function body", function()
            -- a table is not string-coercible (a number would be accepted as code)
            assert.has_error(function() tempTrigger("pat", {}) end)
        end)

        it("tempTrigger rejects an expiration count below one", function()
            local id, err = tempTrigger("pat", [[]], 0)
            if type(id) == "number" and id > 0 then killTrigger(id) end
            assert.is_nil(id, "an expiry count of zero should be rejected")
            assert.is_string(err)
            assert.is_truthy(err:find("greater than zero", 1, true), "got: " .. tostring(err))
        end)

        it("tempTrigger rejects a non-number, non-nil expiration count", function()
            assert.has_error(function() tempTrigger("pat", [[]], "soon") end)
        end)

        it("tempRegexTrigger rejects a non-string, non-function body", function()
            assert.has_error(function() tempRegexTrigger("^pat$", {}) end)
        end)

        it("tempExactMatchTrigger rejects a non-string, non-function body", function()
            assert.has_error(function() tempExactMatchTrigger("pat", true) end)
        end)

        it("tempBeginOfLineTrigger rejects a non-number, non-nil expiration count", function()
            assert.has_error(function() tempBeginOfLineTrigger("pat", [[]], "later") end)
        end)

    end)

    describe("temporary trigger expiration", function()

        after_each(function()
            _G.TrigSpecExpire = nil
        end)

        it("a trigger with expiration count N fires exactly N times", function()
            _G.TrigSpecExpire = {count = 0}
            local id = tempTrigger("expire_me", [[_G.TrigSpecExpire.count = _G.TrigSpecExpire.count + 1]], 2)
            assert.is_number(id)
            -- fed more times than the expiry count: it must stop after the second
            feedTriggers("\nexpire_me\n")
            feedTriggers("\nexpire_me\n")
            feedTriggers("\nexpire_me\n")
            feedTriggers("\nexpire_me\n")
            local count = _G.TrigSpecExpire.count
            _G.TrigSpecExpire = nil
            -- if it somehow survived, make sure it is gone
            if type(id) == "number" and id > 0 then killTrigger(id) end
            assert.is_equal(2, count, "a trigger set to expire after 2 fires should fire exactly twice")
        end)

        -- Regression: the C++ helpers that run trigger/alias/script code used to
        -- read the script's return value from an absolute stack slot and then
        -- wipe the whole shared Lua stack. Running inside feedTriggers() those
        -- slots hold feedTriggers' own arguments, so the Utf8Encoded boolean
        -- below was mistaken for "the script returned true" and kept renewing
        -- the expiry count, and the wipe took the caller's arguments with it.
        it("expires on schedule when fed by a call that has arguments on the Lua stack", function()
            _G.TrigSpecExpire = {count = 0}
            local id = tempTrigger("expire_me_utf8", [[_G.TrigSpecExpire.count = _G.TrigSpecExpire.count + 1]], 1)
            assert.is_number(id)
            feedTriggers("\nexpire_me_utf8\n", true)
            feedTriggers("\nexpire_me_utf8\n", true)
            feedTriggers("\nexpire_me_utf8\n", true)
            local count = _G.TrigSpecExpire.count
            _G.TrigSpecExpire = nil
            if type(id) == "number" and id > 0 then killTrigger(id) end
            assert.is_equal(1, count, "a trigger set to expire after 1 fire must not be renewed by the caller's stack")
        end)

    end)

    describe("tempColorTrigger legacy colour remap", function()

        after_each(function()
            _G.TrigSpec = nil
        end)

        -- tempColorTrigger applies a fossilised 0-16 remap table; argument 4 maps
        -- to Mudlet colour index 1 (red), NOT the ANSI blue that a naive reading
        -- of "4" would suggest. This pins that remap.
        it("remaps its colour arguments and matches the remapped SGR colours", function()
            _G.TrigSpec = {fired = false}
            -- fg arg 4 -> index 1 (red -> SGR 31); bg arg 2 -> index 0 (black -> SGR 40)
            local id = tempColorTrigger(4, 2, function() _G.TrigSpec.fired = true end)
            assert.is_number(id)
            -- blue foreground (SGR 34) must NOT match: proves 4 is not used literally
            feedTriggers("\n\27[34;40mColorTrigNoMatch\27[0m\n")
            local firedOnNonMatch = _G.TrigSpec.fired
            -- red foreground (SGR 31) on black background (SGR 40) must match
            feedTriggers("\n\27[31;40mColorTrigMatch\27[0m\n")
            local fired = _G.TrigSpec.fired
            -- kill before asserting: a leaked colour trigger matches by SGR and
            -- would fire on later ANSI-colour specs' fed lines
            killTrigger(id)
            assert.is_false(firedOnNonMatch, "arg 4 must remap to red, so blue text must not fire it")
            assert.is_true(fired, "the remapped red-on-black colour trigger should fire on red-on-black text")
        end)

        it("rejects ignoring both foreground and background", function()
            local id, err = tempColorTrigger(-1, -1, function() end)
            if type(id) == "number" and id > 0 then killTrigger(id) end
            assert.is_nil(id)
            assert.is_string(err)
            assert.is_truthy(err:find("only one of foreground and background", 1, true), "got: " .. tostring(err))
        end)

        it("rejects a non-string, non-function body", function()
            assert.has_error(function() tempColorTrigger(1, 1, {}) end)
        end)

    end)

    describe("tempComplexRegexTrigger", function()

        after_each(function()
            _G.TrigSpec = nil
        end)

        -- 14-positional-argument API; here as a plain (non-colour, non-highlight,
        -- non-sound) perl regex trigger: multiline 0, fg/bg numeric (so not a
        -- colour trigger), filter 0, matchAll 0, highlight fg/bg numeric, sound
        -- numeric (so no sound), fireLength 0, lineDelta 0.
        it("creates a firing perl regex trigger", function()
            _G.TrigSpec = {fired = false}
            local id = tempComplexRegexTrigger("SpecComplexOne", "^complex_match$",
                function() _G.TrigSpec.fired = true end,
                0, -1, -1, 0, 0, -1, -1, 0, 0, 0)
            assert.is_number(id)
            feedTriggers("\ncomplex_match\n")
            local fired = _G.TrigSpec.fired
            assert.is_true(killTrigger("SpecComplexOne"), "a temporary complex trigger should be removable by name")
            assert.is_true(fired, "a complex regex trigger should fire on its pattern")
        end)

        it("rejects a non-string, non-function body (argument 3)", function()
            -- a table is not string-coercible (a number would be accepted as code)
            assert.has_error(function()
                tempComplexRegexTrigger("SpecComplexBad", "^x$", {}, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0)
            end)
        end)

        it("rejects a non-number multiline flag (argument 4)", function()
            assert.has_error(function()
                tempComplexRegexTrigger("SpecComplexBad", "^x$", [[]], "yes", -1, -1, 0, 0, -1, -1, 0, 0, 0)
            end)
        end)

        it("rejects a non-number filter flag (argument 7)", function()
            assert.has_error(function()
                tempComplexRegexTrigger("SpecComplexBad", "^x$", [[]], 0, -1, -1, "no", 0, -1, -1, 0, 0, 0)
            end)
        end)

        it("rejects a non-number match-all flag (argument 8)", function()
            assert.has_error(function()
                tempComplexRegexTrigger("SpecComplexBad", "^x$", [[]], 0, -1, -1, 0, "no", -1, -1, 0, 0, 0)
            end)
        end)

    end)

    describe("permanent triggers", function()

        -- permanent triggers cannot be removed with killTrigger, so give them
        -- unique names and disable them after each test to avoid leaking firing
        -- state into other specs.
        after_each(function()
            disableTrigger("SpecPermRegex")
            disableTrigger("SpecPermSubstring")
            disableTrigger("SpecPermBeginOfLine")
            disableTrigger("SpecPermExact")
            _G.TrigSpec = nil
        end)

        it("permRegexTrigger creates a firing trigger from a pattern table", function()
            _G.TrigSpec = {fired = false}
            local id = permRegexTrigger("SpecPermRegex", "", {"^perm_regex_hit$"}, [[_G.TrigSpec.fired = true]])
            assert.is_number(id)
            assert.is_true(id > 0)
            feedTriggers("\nperm_regex_hit\n")
            assert.is_true(_G.TrigSpec.fired, "a permanent regex trigger should fire on its pattern")
        end)

        it("permSubstringTrigger creates a firing trigger", function()
            _G.TrigSpec = {fired = false}
            local id = permSubstringTrigger("SpecPermSubstring", "", {"perm_sub_hit"}, [[_G.TrigSpec.fired = true]])
            assert.is_number(id)
            feedTriggers("\nsomething perm_sub_hit here\n")
            assert.is_true(_G.TrigSpec.fired, "a permanent substring trigger should fire when the substring appears")
        end)

        it("permBeginOfLineStringTrigger creates a firing trigger", function()
            _G.TrigSpec = {fired = false}
            local id = permBeginOfLineStringTrigger("SpecPermBeginOfLine", "", {"perm_bol_hit"}, [[_G.TrigSpec.fired = true]])
            assert.is_number(id)
            feedTriggers("\nnope perm_bol_hit\n")
            assert.is_false(_G.TrigSpec.fired, "a begin-of-line trigger must not fire mid-line")
            feedTriggers("\nperm_bol_hit at start\n")
            assert.is_true(_G.TrigSpec.fired, "a begin-of-line trigger should fire when the string starts the line")
        end)

        it("permExactMatchTrigger creates a firing trigger", function()
            _G.TrigSpec = {count = 0}
            local id = permExactMatchTrigger("SpecPermExact", "", {"perm_exact_hit"}, [[_G.TrigSpec.count = _G.TrigSpec.count + 1]])
            assert.is_number(id)
            feedTriggers("\nperm_exact_hit trailing\n")
            assert.is_equal(0, _G.TrigSpec.count, "an exact-match trigger must not fire on a superset line")
            feedTriggers("\nperm_exact_hit\n")
            assert.is_equal(1, _G.TrigSpec.count, "an exact-match trigger should fire on the exact line")
        end)

        it("permRegexTrigger errors when the pattern argument is not a table", function()
            assert.has_error(function()
                permRegexTrigger("SpecPermRegex", "", "^not a table$", [[]])
            end)
        end)

        it("permSubstringTrigger errors when the pattern argument is not a table", function()
            assert.has_error(function()
                permSubstringTrigger("SpecPermSubstring", "", "not a table", [[]])
            end)
        end)

        -- Guards that the error names this function (a past audit flagged a
        -- sibling message that misreported itself as "permRegexTrigger").
        it("permBeginOfLineStringTrigger errors when the pattern argument is not a table", function()
            local ok, err = pcall(function()
                permBeginOfLineStringTrigger("SpecPermBeginOfLine", "", "not a table", [[]])
            end)
            assert.is_false(ok, "a non-table pattern argument should error")
            assert.is_truthy(tostring(err):find("permBeginOfLineStringTrigger", 1, true),
                "the error should name permBeginOfLineStringTrigger, got: " .. tostring(err))
        end)

        it("killTrigger returns false for a permanent trigger (they cannot be killed)", function()
            local id = permRegexTrigger("SpecPermRegex", "", {"^perm_kill_probe$"}, [[]])
            assert.is_true(id > 0)
            assert.is_false(killTrigger("SpecPermRegex"), "permanent triggers cannot be removed with killTrigger")
        end)

    end)

    describe("enable, disable, isActive and exists for triggers", function()

        after_each(function()
            _G.TrigSpec = nil
        end)

        it("disableTrigger stops a trigger firing and enableTrigger restores it", function()
            _G.TrigSpec = {count = 0}
            local id = tempRegexTrigger("^toggle_me$", function() _G.TrigSpec.count = _G.TrigSpec.count + 1 end)
            feedTriggers("\ntoggle_me\n")
            assert.is_equal(1, _G.TrigSpec.count, "an enabled trigger should fire")

            assert.is_true(disableTrigger(id), "disableTrigger should report it found the trigger")
            feedTriggers("\ntoggle_me\n")
            assert.is_equal(1, _G.TrigSpec.count, "a disabled trigger must not fire")

            assert.is_true(enableTrigger(id), "enableTrigger should report it found the trigger")
            feedTriggers("\ntoggle_me\n")
            assert.is_equal(2, _G.TrigSpec.count, "a re-enabled trigger should fire again")

            killTrigger(id)
        end)

        it("enableTrigger and disableTrigger return false for an unknown name", function()
            assert.is_false(enableTrigger("no_such_trigger_name"))
            assert.is_false(disableTrigger("no_such_trigger_name"))
        end)

        it("isActive reflects a trigger's enabled state", function()
            local id = tempRegexTrigger("^active_probe$", [[]])
            assert.is_equal(1, isActive(id, "trigger"), "a freshly created trigger should be active")
            disableTrigger(id)
            assert.is_equal(0, isActive(id, "trigger"), "a disabled trigger should not be active")
            enableTrigger(id)
            assert.is_equal(1, isActive(id, "trigger"), "a re-enabled trigger should be active")
            killTrigger(id)
        end)

        it("isActive rejects an invalid item type", function()
            local ok, err = isActive(1, "notarealtype")
            assert.is_nil(ok)
            assert.is_string(err)
            assert.is_truthy(err:find("invalid item type", 1, true), "got: " .. tostring(err))
        end)

        it("isActive rejects a negative numeric ID", function()
            local ok, err = isActive(-5, "trigger")
            assert.is_nil(ok)
            assert.is_string(err)
        end)

        it("exists round-trips a temporary trigger by ID", function()
            local id = tempRegexTrigger("^exists_probe$", [[]])
            assert.is_equal(1, exists(id, "trigger"), "the trigger should exist after creation")
            assert.is_true(killTrigger(id), "killTrigger should report success for a temporary trigger")
            -- feed a line to let the trigger unit run its deferred cleanup
            feedTriggers("\nflush\n")
            assert.is_equal(0, exists(id, "trigger"), "the trigger should be gone after kill and cleanup")
        end)

        it("killTrigger returns false for a name that does not exist", function()
            assert.is_false(killTrigger("no_such_trigger_name"))
        end)

        it("killTrigger returns false the second time, as the trigger is already dead", function()
            local id = tempRegexTrigger("^double_kill_probe$", [[]])
            assert.is_true(killTrigger(id), "killing a live temporary trigger should report success")
            -- the trigger is still present here: only the deferred cleanup frees it,
            -- so the second kill really is being told about a corpse it can find
            assert.is_equal(1, exists(id, "trigger"), "the killed trigger is still present until cleanup runs")
            assert.is_equal(0, isActive(id, "trigger"), "a killed trigger is no longer active")
            assert.is_false(killTrigger(id),
                "killing an already killed trigger achieves nothing and has to say so")
            -- a fed line runs that cleanup, and the answer has to be the same after it
            feedTriggers("\ndouble_kill_flush\n")
            assert.is_equal(0, exists(id, "trigger"), "the trigger should be gone after kill and cleanup")
            assert.is_false(killTrigger(id), "a freed trigger cannot be killed either")
        end)

        it("a trigger killed earlier in a line's pass does not fire on that line", function()
            _G.TrigSpec = {count = 0, witness = 0}
            -- the killer is created first, so the trigger unit reaches it first and
            -- its victim is still in the list this pass is walking; only the cleanup
            -- at the end of the line frees the victim. The witness is created last so
            -- that it proves the pass really did carry on past the killer
            local victimId
            local killerId = tempRegexTrigger("^kill_stops_firing$", function()
                _G.TrigSpec.killed = killTrigger(victimId)
            end)
            victimId = tempRegexTrigger("^kill_stops_firing$", function()
                _G.TrigSpec.count = _G.TrigSpec.count + 1
            end)
            local witnessId = tempRegexTrigger("^kill_stops_firing$", function()
                _G.TrigSpec.witness = _G.TrigSpec.witness + 1
            end)
            feedTriggers("\nkill_stops_firing\n")
            killTrigger(killerId)
            killTrigger(witnessId)
            assert.is_true(_G.TrigSpec.killed, "the first trigger should have killed the second")
            assert.is_equal(1, _G.TrigSpec.witness, "the line should still reach triggers behind the killer")
            assert.is_equal(0, _G.TrigSpec.count,
                "a killed trigger must no more fire on the rest of the line than a disabled one does")
        end)

        it("killTrigger returns false for a trigger that has used up its last firing", function()
            -- an expiring trigger queues itself for the same deferred cleanup a killed
            -- one does, so it is just as dead - as killTimer reports for a one-shot
            -- timer that has already fired
            _G.TrigSpec = {}
            local expiringId = tempRegexTrigger("^expiry_kill_probe$", [[]], 1)
            local killerId = tempRegexTrigger("^expiry_kill_probe$", function()
                _G.TrigSpec.killedExpired = killTrigger(expiringId)
            end)
            feedTriggers("\nexpiry_kill_probe\n")
            killTrigger(killerId)
            assert.is_not_nil(_G.TrigSpec.killedExpired, "the killing trigger should have fired")
            assert.is_false(_G.TrigSpec.killedExpired,
                "a trigger that just used up its last firing cannot be killed again")
        end)

        it("killTrigger returns false the second time inside the trigger's own script", function()
            _G.TrigSpec = {}
            local id
            id = tempRegexTrigger("^self_kill_probe$", function()
                _G.TrigSpec.killed = killTrigger(id)
                _G.TrigSpec.killedAgain = killTrigger(id)
            end)
            feedTriggers("\nself_kill_probe\n")
            assert.is_not_nil(_G.TrigSpec.killed, "the trigger should have fired")
            assert.is_true(_G.TrigSpec.killed,
                "killTrigger should report success from inside the trigger's own script")
            assert.is_false(_G.TrigSpec.killedAgain,
                "killing the same trigger twice from its own script must fail the second time")
        end)

        it("exists rejects an invalid item type", function()
            local ok, err = exists(1, "notarealtype")
            assert.is_nil(ok)
            assert.is_string(err)
            assert.is_truthy(err:find("invalid item type", 1, true), "got: " .. tostring(err))
        end)

    end)

    -- enableTrigger/disableTrigger must toggle EVERY trigger sharing a name, not
    -- just the first (QMultiMap equal_range fix, PR #9366).
    describe("enable/disable with duplicate trigger names", function()

        before_each(function()
            _G.DuplicateTrigTest = {fired = {}}
        end)

        after_each(function()
            -- permanent triggers cannot be killed; disable so they stop firing
            disableTrigger("Spec Dup Triggers")
            disableTrigger("Spec Other Trigger")
            _G.DuplicateTrigTest = nil
        end)

        it("toggles every trigger sharing the same name, not just the first", function()
            local id1 = permRegexTrigger("Spec Dup Triggers", "", {"^trig_dup_one$"}, [[_G.DuplicateTrigTest.fired.one = true]])
            local id2 = permRegexTrigger("Spec Dup Triggers", "", {"^trig_dup_two$"}, [[_G.DuplicateTrigTest.fired.two = true]])
            assert.is_true(id1 > 0, "first duplicate-named trigger should be created")
            assert.is_true(id2 > 0, "second duplicate-named trigger should be created")
            -- by-name count paths (exists/isActive) walk the whole equal_range;
            -- assert invariants rather than exact counts, since permanent triggers
            -- from earlier local runs accumulate under this name in the profile
            assert.is_true(exists("Spec Dup Triggers", "trigger") >= 2, "at least the two duplicates exist under this name")

            feedTriggers("\ntrig_dup_one\n")
            feedTriggers("\ntrig_dup_two\n")
            assert.is_true(_G.DuplicateTrigTest.fired.one, "trigger 1 should fire while enabled")
            assert.is_true(_G.DuplicateTrigTest.fired.two, "trigger 2 should fire while enabled")

            _G.DuplicateTrigTest.fired = {}
            disableTrigger("Spec Dup Triggers")
            assert.is_equal(0, isActive("Spec Dup Triggers", "trigger"), "disabling by name must leave zero of the duplicates active")
            feedTriggers("\ntrig_dup_one\n")
            feedTriggers("\ntrig_dup_two\n")
            assert.is_nil(_G.DuplicateTrigTest.fired.one, "trigger 1 should be disabled")
            assert.is_nil(_G.DuplicateTrigTest.fired.two, "trigger 2 (the second duplicate) should ALSO be disabled")

            _G.DuplicateTrigTest.fired = {}
            enableTrigger("Spec Dup Triggers")
            -- enabling by name reactivates every same-named trigger, so the active
            -- count equals the total count regardless of how many accumulated
            assert.is_equal(exists("Spec Dup Triggers", "trigger"), isActive("Spec Dup Triggers", "trigger"),
                "enabling by name must reactivate every duplicate")
            feedTriggers("\ntrig_dup_one\n")
            feedTriggers("\ntrig_dup_two\n")
            assert.is_true(_G.DuplicateTrigTest.fired.one, "trigger 1 should be re-enabled")
            assert.is_true(_G.DuplicateTrigTest.fired.two, "trigger 2 (the second duplicate) should ALSO be re-enabled")

            -- a differently-named trigger must stay untouched
            local idOther = permRegexTrigger("Spec Other Trigger", "", {"^trig_dup_other$"}, [[_G.DuplicateTrigTest.fired.other = true]])
            assert.is_true(idOther > 0)
            disableTrigger("Spec Dup Triggers")
            _G.DuplicateTrigTest.fired = {}
            feedTriggers("\ntrig_dup_other\n")
            assert.is_true(_G.DuplicateTrigTest.fired.other, "the differently-named trigger must stay enabled")
        end)

    end)

    describe("setTriggerStayOpen", function()

        it("requires a numeric number-of-lines argument", function()
            assert.has_error(function() setTriggerStayOpen("main", "lots") end)
        end)

        it("accepts a window name and a line count without error", function()
            -- returns nothing; success is simply not raising
            local ok = pcall(setTriggerStayOpen, "main", 0)
            assert.is_true(ok, "setTriggerStayOpen with valid arguments should not error")
        end)

    end)

    -- Prompt pipeline. Prompt-line *firing* would need feedTelnet to inject an
    -- IAC GA, but feedTelnet is refused here because the self-test socket is not
    -- in the unconnected state (see the feedTelnet note above), so only the
    -- creation contracts and isPrompt's shape are exercised offline.
    describe("prompt triggers and isPrompt", function()

        after_each(function()
            disableTrigger("SpecPermPrompt")
            _G.TrigSpec = nil
        end)

        it("isPrompt reports false when no prompt has been marked", function()
            -- no IAC GA is ever injected in this profile, so the current line is
            -- never a prompt
            assert.is_false(isPrompt())
        end)

        it("tempPromptTrigger creates a trigger from a function body", function()
            local id = tempPromptTrigger(function() end)
            assert.is_number(id)
            assert.are.equal(1, exists(id, "trigger"))
            killTrigger(id)
        end)

        it("permPromptTrigger creates a trigger and returns its id", function()
            -- relative baseline: permanent triggers from earlier local runs
            -- accumulate under this name in the saved profile
            local before = exists("SpecPermPrompt", "trigger")
            local id = permPromptTrigger("SpecPermPrompt", "", [[]])
            assert.is_number(id)
            assert.is_true(id > 0)
            assert.are.equal(before + 1, exists("SpecPermPrompt", "trigger"), "creating the prompt trigger should add exactly one")
        end)

        it("tempPromptTrigger rejects a non-string, non-function body", function()
            -- a table is not string-coercible (a number would be accepted as code)
            assert.has_error(function() tempPromptTrigger({}) end)
        end)

        it("tempPromptTrigger rejects an expiration count below one", function()
            local id, err = tempPromptTrigger([[]], 0)
            if type(id) == "number" and id > 0 then killTrigger(id) end
            assert.is_nil(id)
            assert.is_string(err)
            assert.is_truthy(err:find("greater than zero", 1, true), "got: " .. tostring(err))
        end)

    end)

    -- The delete of an expired or killed trigger is deferred until the outermost
    -- processDataStream() pass ends, so everything between the queueing and the
    -- free has to behave as if the trigger were already gone.
    describe("deferred deletion", function()

        it("does not let an expired trigger fire again from a nested feed", function()
            local fires = 0
            -- expireAfter = 1, so this must fire exactly once no matter how many
            -- lines reach it
            tempRegexTrigger("^expiry_reentry$", function() fires = fires + 1 end, 1)

            local nestedFed = false
            local reentrantId = tempRegexTrigger("^expiry_reentry$", function()
                if not nestedFed then
                    nestedFed = true
                    -- re-enters trigger processing while the expired trigger is
                    -- still queued for deletion
                    feedTriggers("\nexpiry_reentry\n")
                end
            end)
            finally(function() killTrigger(reentrantId) end)

            feedTriggers("\nexpiry_reentry\n")

            assert.is_true(nestedFed, "the re-entrant trigger should have fed a nested line")
            assert.are.equal(1, fires, "a trigger with expireAfter = 1 must not fire a second time")
        end)

        it("does not let an expiring trigger fire again from its own nested feed", function()
            -- A separate defect from the one above, found while fixing it: the
            -- expiry count is decremented at the end of match(), after execute()
            -- has run, so a trigger whose own script re-feeds the matching line is
            -- still at its old count and still active when the nested pass reaches
            -- it. Fixing that means moving the expiry accounting ahead of
            -- execute(), which also has to keep the "return true to extend the
            -- expiry" contract working - out of scope for the deactivate() fix.
            pending("expiry is accounted after execute(), so a self-refeeding trigger overshoots expireAfter")
            local fires = 0
            local nestedFed = false
            tempRegexTrigger("^self_expiry_reentry$", function()
                fires = fires + 1
                if not nestedFed then
                    nestedFed = true
                    feedTriggers("\nself_expiry_reentry\n")
                end
            end, 1)

            feedTriggers("\nself_expiry_reentry\n")

            assert.is_true(nestedFed)
            assert.are.equal(1, fires, "a trigger with expireAfter = 1 must not fire a second time")
        end)

        it("does not let enableTrigger revive a trigger that is waiting to be freed", function()
            -- a killed trigger stays findable by name until the deferred delete
            -- runs, so enabling it again would resurrect it
            local name = "Spec Enable Resurrection"
            _G.EnableResurrectionSpec = 0
            finally(function() _G.EnableResurrectionSpec = nil end)

            tempComplexRegexTrigger(name, "^enable_resurrection$",
                [[_G.EnableResurrectionSpec = _G.EnableResurrectionSpec + 1]], 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
            assert.is_true(killTrigger(name))
            assert.is_false(enableTrigger(name), "a killed trigger must not be re-enabled before it is freed")

            feedTriggers("\nenable_resurrection\n")

            assert.are.equal(0, _G.EnableResurrectionSpec, "a killed trigger must not fire, whatever enableTrigger was told")
        end)

        it("keeps a same-named permanent trigger in the lookup table", function()
            local name = "Spec Name Eviction"
            _G.NameEvictionSpec = 0
            finally(function()
                disableTrigger(name)
                _G.NameEvictionSpec = nil
            end)

            -- permanent triggers cannot be deleted from Lua, so earlier local runs
            -- leave same-named ones behind: work from a relative baseline
            assert.is_true(permRegexTrigger(name, "", {"^name_eviction_perm$"}, [[_G.NameEvictionSpec = (_G.NameEvictionSpec or 0) + 1]]) > 0)
            local permanents = exists(name, "trigger")
            assert.is_true(permanents >= 1)

            -- tempComplexRegexTrigger is the one temporary-trigger API that takes a
            -- user-supplied name, so sharing one with a permanent trigger is easy.
            -- Note it copies the pattern list of the trigger it finds under that
            -- name, so this temporary also carries ^name_eviction_perm$ - harmless
            -- here, since it is killed before anything is fed
            tempComplexRegexTrigger(name, "^name_eviction_temp$", [[]], 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
            assert.are.equal(permanents + 1, exists(name, "trigger"))

            killTrigger(name) -- only the temporary one can be killed
            feedTriggers("\nname_eviction_perm\n") -- the pass ends, flushing the deferred delete

            assert.are.equal(permanents, exists(name, "trigger"), "only the temporary trigger should leave the lookup table")
            assert.is_true(_G.NameEvictionSpec >= 1, "the permanent trigger should still fire")
            assert.is_true(disableTrigger(name), "the permanent trigger must still be reachable by name")
        end)

    end)
end)
