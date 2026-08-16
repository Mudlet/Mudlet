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

        -- A trigger script that calls feedTriggers() re-enters trigger processing.
        -- The outer and inner lines are deliberately different lengths: an equal
        -- length lets the colour scan window line up by luck, which is why the
        -- spec above passes even without the fix.
        local outerLine = "OuterMarkerLineIsLonger"
        local innerLine = "InnerShort"

        it("should still match later triggers against the outer line after a nested feedTriggers", function()
            _G.reentrancySubstring = false
            local feeder = tempRegexTrigger("^" .. outerLine .. "$", function()
                feedTriggers("\n" .. innerLine .. "\n")
            end)
            -- a substring trigger reads the haystack string itself, so it is the
            -- kind the aliasing silenced; "Marker" is only in the outer line
            local watcher = tempTrigger("Marker", function()
                _G.reentrancySubstring = true
            end)

            feedTriggers("\n" .. outerLine .. "\n")

            local fired = _G.reentrancySubstring
            killTrigger(feeder)
            killTrigger(watcher)
            _G.reentrancySubstring = nil

            assert.is_true(fired, "A substring trigger should still see the outer line after an earlier trigger fed text")
        end)

        it("should leave the outer line selectable after a nested feedTriggers", function()
            _G.reentrancySelect = -1
            local feeder = tempRegexTrigger("^" .. outerLine .. "$", function()
                feedTriggers("\n" .. innerLine .. "\n")
            end)
            -- a regex trigger matches whatever the haystack is, so what this pins
            -- is which line the cursor is left pointing at
            local selector = tempRegexTrigger("Marker", function()
                _G.reentrancySelect = selectString("Marker", 1)
            end)

            feedTriggers("\n" .. outerLine .. "\n")

            local selected = _G.reentrancySelect
            killTrigger(feeder)
            killTrigger(selector)
            _G.reentrancySelect = nil

            assert.is_true(selected > -1, "selectString should still find the outer line, not the line that was fed")
        end)

        it("should restore the line variable after a nested feedTriggers", function()
            _G.reentrancyLine = nil
            local feeder = tempRegexTrigger("^" .. outerLine .. "$", function()
                feedTriggers("\n" .. innerLine .. "\n")
            end)
            local reader = tempRegexTrigger("Marker", function()
                _G.reentrancyLine = line
            end)

            feedTriggers("\n" .. outerLine .. "\n")

            local seen = _G.reentrancyLine
            killTrigger(feeder)
            killTrigger(reader)
            _G.reentrancyLine = nil

            assert.are.equal(outerLine, seen)
        end)

        -- printOnDisplay() is where a nested feed lands, so clearing its trigger
        -- flag outright leaves the rest of the outer pass outside trigger context.
        -- What that flag gates is not whether inserted text lands inline - the
        -- ordinary path inserts inline too whenever the cursor is not at the end
        -- of the buffer - but whether the capture positions are shifted to follow
        -- it. Without that, a capture selected after an insert is off by the
        -- length inserted.
        it("should keep captures selectable after an insert across a nested feedTriggers", function()
            _G.reentrancyCapture = nil
            -- unanchored: this line carries the marker text after the outer line
            local feeder = tempRegexTrigger("^" .. outerLine, function()
                feedTriggers("\n" .. innerLine .. "\n")
            end)
            -- selectCaptureGroup(1) selects the whole match, so that is what the
            -- insert has to leave correctly positioned
            local inserter = tempRegexTrigger("Marker (\\w+)$", function()
                moveCursor(0, getLineNumber())
                insertText("ZZZZ")
                selectCaptureGroup(1)
                _G.reentrancyCapture = getSelection()
            end)

            feedTriggers("\n" .. outerLine .. " Marker payload\n")

            local selected = _G.reentrancyCapture
            killTrigger(feeder)
            killTrigger(inserter)
            _G.reentrancyCapture = nil

            assert.are.equal("Marker payload", selected,
                "the match should still be selectable after text was inserted before it")
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
    -- otherwise it refuses with nil + a message. That is the state --offline
    -- leaves the profile in, so the injection path runs here against the real
    -- telnet parser.
    describe("feedTelnet contract", function()

        local function feedOrExplain(data)
            local ok, msg = feedTelnet(data)
            assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
        end

        -- every injection below leaves the buffer somewhere the specs after it
        -- read from, and an assert that throws would otherwise skip the restore
        after_each(function()
            deselect()
            feedTelnet("\r\n")
            setConfig("specialForceGAOff", false)
        end)

        it("raises an error when the data argument is not a string", function()
            -- a table is never string-coercible, unlike a number
            assert.has_error(function() feedTelnet({}) end)
        end)

        it("runs against a profile that reports no connection", function()
            local _, _, connected = getConnectionInfo()
            assert.is_false(connected, "getConnectionInfo() must not report an established connection")
        end)

        it("injects server data for the telnet parser to decode", function()
            -- reading the colour back proves the bytes travelled through the
            -- telnet parser rather than being appended as plain text
            feedOrExplain("\27[31mSpecFedRedLine\27[0m\r\n")
            assert.is_true(selectString("SpecFedRedLine", 1) >= 0, "the injected line did not reach the buffer")
            assert.are.same(color_table.ansi_red, {getFgColor()})
        end)

        it("treats a line ended by IAC GA as a prompt", function()
            feedOrExplain("SpecFedPrompt> <T_IAC><T_GA>")
            assert.is_true(isPrompt(), "IAC GA should have marked the fed line a prompt")
        end)

        it("stops honouring IAC GA once the profile forces GA off", function()
            setConfig("specialForceGAOff", true)
            feedOrExplain("SpecGaForcedOff> <T_IAC><T_GA>")
            assert.is_false(isPrompt(), "a forced-off GA must not mark the fed line a prompt")
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
            disableTrigger("SpecPermBlankPattern")
            disableTrigger("SpecPermTwoBlankPatterns")
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

        -- Blank patterns are skipped when the pattern list is stored, so the stored
        -- list is compacted while the list passed in is not, and a pattern following
        -- a blank one must still be matched with its own regex. A pattern table with
        -- more than one entry makes the trigger multiline, so its captures arrive in
        -- multimatches rather than matches even though only one pattern survives.
        -- A permanent trigger saved by an earlier run reloads with its blank pattern
        -- already stripped, i.e. healed, so it has to be cleared out first or it
        -- fires and masks the result.
        it("a blank pattern before a real one does not stop the real one matching", function()
            _G.TrigSpec = {capture = nil}
            killTrigger("SpecPermBlankPattern")
            disableTrigger("SpecPermBlankPattern")
            local id = permRegexTrigger("SpecPermBlankPattern", "", {"", "^You see (\\w+)$"},
                                        [==[_G.TrigSpec.capture = multimatches[1][2]]==])
            assert.is_number(id)
            assert.is_true(id > 0)
            feedTriggers("\nYou see sword\n")
            assert.is_equal("sword", _G.TrigSpec.capture,
                "a pattern after a blank one should match and report its own capture")
        end)

        it("two blank patterns before a real one do not shift its regex", function()
            _G.TrigSpec = {capture = nil}
            killTrigger("SpecPermTwoBlankPatterns")
            disableTrigger("SpecPermTwoBlankPatterns")
            local id = permRegexTrigger("SpecPermTwoBlankPatterns", "", {"", "", "^probe (\\w+)$"},
                                        [==[_G.TrigSpec.capture = multimatches[1][2]]==])
            assert.is_number(id)
            feedTriggers("\nprobe alpha\n")
            assert.is_equal("alpha", _G.TrigSpec.capture,
                "the shift should be corrected for any number of leading blank patterns")
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

    -- Prompt pipeline. Only the creation contracts are exercised here; a prompt
    -- trigger *firing* is still untested, and the feedTelnet block above shows
    -- how to inject the IAC GA that would need.
    describe("prompt triggers and isPrompt", function()

        after_each(function()
            disableTrigger("SpecPermPrompt")
            _G.TrigSpec = nil
        end)

        it("isPrompt reports false when the cursor is not on a prompt line", function()
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

    -- Multiline triggers: a state completes only once every pattern has matched
    -- within the condition line delta. tempComplexRegexTrigger() is the only Lua
    -- function that can set that delta, and calling it twice under one name
    -- accumulates the patterns into a single trigger - so each spec below uses a
    -- name of its own, because a second call against a surviving trigger would
    -- add a third condition rather than start afresh.
    describe("multiline trigger state", function()

        it("fires once with the captures from both of its lines", function()
            _G.MLCaps = {fires = 0, caps = ""}
            local code = [==[
                _G.MLCaps.fires = _G.MLCaps.fires + 1
                _G.MLCaps.caps = multimatches[1][2] .. "," .. multimatches[2][2]
            ]==]
            tempComplexRegexTrigger("SpecMLCaps", [[^one (\w+)$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 3)
            tempComplexRegexTrigger("SpecMLCaps", [[^two (\w+)$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 3)

            feedTriggers("one aaa\n")
            feedTriggers("two bbb\n")

            local fires, caps = _G.MLCaps.fires, _G.MLCaps.caps
            killTrigger("SpecMLCaps")
            _G.MLCaps = nil
            assert.are.equal(1, fires)
            assert.are.equal("aaa,bbb", caps, "both lines' captures should reach the script through multimatches")
        end)

        -- Both sides are asserted: a count of zero on its own is also what a
        -- silently broken setup produces.
        it("drops a state whose line delta has run out", function()
            _G.MLExpiry = 0
            local code = [==[_G.MLExpiry = _G.MLExpiry + 1]==]
            tempComplexRegexTrigger("SpecMLExpiry", [[^first$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1)
            tempComplexRegexTrigger("SpecMLExpiry", [[^second$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1)

            feedTriggers("first\n")
            feedTriggers("second\n")
            local consecutive = _G.MLExpiry
            feedTriggers("first\n")
            feedTriggers("padding\n")
            feedTriggers("second\n")
            local padded = _G.MLExpiry

            killTrigger("SpecMLExpiry")
            _G.MLExpiry = nil
            assert.are.equal(1, consecutive, "a state within its line delta should complete")
            assert.are.equal(1, padded, "a state whose delta has run out should not be completed by a later line")
        end)

        it("passes a filtering trigger's captures to its children", function()
            _G.MLFilter = {}
            tempComplexRegexTrigger("SpecMLFilterParent", [[^hit (\w+) for (\d+)$]], [==[ ]==], 0, 0, 0, 1, 0, 0, 0, 0, 0, 0)
            permRegexTrigger("SpecMLFilterChild", "SpecMLFilterParent", {[[(\w+)]]},
                             [==[table.insert(_G.MLFilter, matches[2])]==])

            feedTriggers("hit orc for 12\n")

            local seen = table.concat(_G.MLFilter, ",")
            killTrigger("SpecMLFilterChild")
            killTrigger("SpecMLFilterParent")
            _G.MLFilter = nil
            assert.are.equal("orc,12", seen, "a filter trigger should offer each of its captures to its children")
        end)

        it("fires a state created and completed inside a nested feed exactly once", function()
            _G.MLNested = 0
            tempComplexRegexTrigger("SpecMLNestedOuter", [[^outer$]], [===[
                tempComplexRegexTrigger("SpecMLNestedInner", [[^alpha$]], [==[_G.MLNested = _G.MLNested + 1]==], 1, 0, 0, 0, 0, 0, 0, 0, 0, 3)
                tempComplexRegexTrigger("SpecMLNestedInner", [[^beta$]], [==[_G.MLNested = _G.MLNested + 1]==], 1, 0, 0, 0, 0, 0, 0, 0, 0, 3)
                feedTriggers("alpha\n")
                feedTriggers("beta\n")
            ]===], 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)

            feedTriggers("outer\n")

            local fires = _G.MLNested
            killTrigger("SpecMLNestedInner")
            killTrigger("SpecMLNestedOuter")
            _G.MLNested = nil
            assert.are.equal(1, fires, "a state created and completed within a nested pass should fire once, in that pass")
        end)

        it("fires both states that complete on the same line", function()
            _G.MLDrain = 0
            local code = [==[_G.MLDrain = _G.MLDrain + 1]==]
            tempComplexRegexTrigger("SpecMLDrain", [[^ss$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 5)
            tempComplexRegexTrigger("SpecMLDrain", [[^tt$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 5)

            -- two "ss" lines leave two states each waiting for "tt", so the "tt"
            -- line completes both of them at once
            feedTriggers("ss\n")
            feedTriggers("ss\n")
            feedTriggers("tt\n")

            local fires = _G.MLDrain
            killTrigger("SpecMLDrain")
            _G.MLDrain = nil
            assert.are.equal(2, fires, "every state completing on one line should fire, not just the first")
        end)

        it("keeps firing for the length set by fireLength", function()
            _G.MLKeep = 0
            local code = [==[_G.MLKeep = _G.MLKeep + 1]==]
            tempComplexRegexTrigger("SpecMLKeep", [[^go$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 2, 3)
            tempComplexRegexTrigger("SpecMLKeep", [[^now$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 2, 3)

            feedTriggers("go\n")
            feedTriggers("now\n")
            feedTriggers("filler\n")
            feedTriggers("filler\n")

            local fires = _G.MLKeep
            killTrigger("SpecMLKeep")
            _G.MLKeep = nil
            assert.are.equal(3, fires, "fireLength should keep the trigger firing on the lines after it completed")
        end)

    end)

    -- A colour-pattern trigger that is the child of a filter parent ("only pass
    -- matches") has to fire when the filtered capture carries the wanted colours,
    -- and only scan that capture. No Lua API builds that tree - only the perm*
    -- family takes a parent name, and none of them makes a colour pattern - so
    -- it comes out of a package fixture. A perl child rides along in the same
    -- fixture: it is measured rather than scanned, which is what the multibyte
    -- case below needs.
    describe("colour trigger children of a filter parent", function()

        local packageName = "mudlet-spec-colorfilter"
        local specDirectory = debug.getinfo(1, "S").source:match("^@(.*)[/\\]")
        assert(specDirectory, "Trigger_spec.lua has to be run from a file so that it can find its fixtures")
        local fixture = specDirectory .. "/fixtures/packages/sources/" .. packageName .. "/" .. packageName .. ".xml"

        if not os.getenv("MUDLET_TEST_MODE") then
            it("needs test mode", function()
                pending("installing the filter-tree fixture needs MUDLET_TEST_MODE (pumpEvents() does nothing without it)")
            end)
            return
        end

        local function packageInstalled()
            for _, entry in ipairs(getPackages()) do
                if entry == packageName then
                    return true
                end
            end
            return false
        end

        local function waitUntil(condition, milliseconds)
            local waited = 0
            while waited < milliseconds do
                if condition() then
                    return true
                end
                pumpEvents(50)
                waited = waited + 50
            end
            return condition() and true or false
        end

        -- Uninstalling queues an asynchronous profile save, and while a save is
        -- running an uninstall is refused outright and an install is postponed,
        -- so both are asked again until they take. A postponed install answers
        -- even a path it would otherwise refuse with a bare true, which is how a
        -- spec can tell a save is still going - at the price of one console
        -- error line per probe when the save ends and the empty path is retried.
        local function waitForProfileSaveToPass()
            return waitUntil(function() return installPackage("") == nil end, 5000)
        end

        local previousEncoding

        setup(function()
            -- the multibyte block below is not encoding agnostic: feedTriggers
            -- transcodes its UTF-8 argument into the server encoding, and under
            -- anything else the capture stops being multibyte at all
            previousEncoding = getServerEncoding()
            assert.is_true(setServerEncoding("UTF-8"), "the profile has to be able to carry the multibyte capture below")
            _G.ColorFilterSpec = {}
            local reason
            for _ = 1, 3 do
                if packageInstalled() then
                    break
                end
                waitForProfileSaveToPass()
                local _, message = installPackage(fixture)
                reason = message or reason
                pumpEvents(200)
            end
            assert.is_true(packageInstalled(), "could not install the " .. packageName .. " fixture: " .. tostring(reason))
        end)

        teardown(function()
            if previousEncoding then
                setServerEncoding(previousEncoding)
            end
            local reason
            for _ = 1, 3 do
                if not packageInstalled() then
                    break
                end
                waitForProfileSaveToPass()
                local _, message = uninstallPackage(packageName)
                reason = message or reason
                pumpEvents(200)
            end
            assert.is_false(packageInstalled(), "the " .. packageName .. " fixture was left behind: " .. tostring(reason))
            _G.ColorFilterSpec = nil
        end)

        before_each(function()
            _G.ColorFilterSpec = {}
        end)

        -- feedTriggers refuses text the server encoding cannot carry, which
        -- would leave the assertions below reading a line that never arrived
        local function feed(text)
            local ok, message = feedTriggers(text)
            assert.is_true(ok, "feedTriggers refused: " .. tostring(message))
        end

        it("fires a top-level colour trigger on the colour it was given", function()
            local id = tempAnsiColorTrigger(3, -1, [[_G.ColorFilterSpec.topLevelFired = true]])
            feed("\27[33mtop level control\27[0m\n")
            killTrigger(id)
            assert.is_true(_G.ColorFilterSpec.topLevelFired, "a top-level colour trigger should fire on yellow text")
        end)

        it("fires a colour child on a filter parent's yellow capture", function()
            feed("\27[33mhello world\27[0m\n")
            assert.is_true(_G.ColorFilterSpec.filterChildFired,
                "a colour trigger child of a filter parent should fire on a yellow capture")
        end)

        it("scans only the parent's capture, not the whole line", function()
            -- only "hello" is yellow while the parent passes just "world" on, so
            -- the child must see no yellow at all
            feed("\27[33mhello\27[0m world\n")
            assert.is_nil(_G.ColorFilterSpec.captureChildFired,
                "a colour child must not fire when the yellow text lies outside the parent's capture")

            feed("hello \27[33mworld\27[0m\n")
            assert.is_true(_G.ColorFilterSpec.captureChildFired,
                "a colour child should fire when the parent's capture is yellow")
        end)

        it("offers a perl child the whole of a multibyte capture", function()
            -- the child's pattern is anchored at both ends, so it only matches if
            -- the capture it is offered runs to the end. The dragon is four UTF-8
            -- bytes but two UTF-16 units (what QString::length() counts), and the
            -- Cyrillic two bytes each, so a length taken in units rather than
            -- bytes would stop the child short of its anchor.
            feed("Цель: 🐉 Оружие: меч\n")
            assert.are.equal("меч", _G.ColorFilterSpec.perlChildWeapon)
        end)

    end)

    -- A trigger created from another trigger's script (tempTrigger() & Co.) still
    -- gets to match the line being processed - room-capture scripts depend on it -
    -- and a lineage of such triggers that keeps re-creating itself is stopped
    -- rather than left to freeze the profile.
    --
    -- Block order is load-bearing. TriggerUnit reports an abort to the console at
    -- most once every ten seconds, so every block that asserts the report is
    -- absent runs before the first runaway, and the single block that reads the
    -- report is that first runaway - of the whole suite, since nothing else in it
    -- makes one. The later runaways read the stopped lineage itself instead,
    -- which is not throttled. If anything ever does trip the budget earlier, the
    -- absence assertions go quietly vacuous and "names the trigger in the abort
    -- report" is what fails to say so.
    describe("triggers created while a line is being processed", function()

        -- TriggerUnit::scmMaxSameLineGenerations, which has no Lua accessor
        local maxGenerations = 1000
        local abortReport = "Trigger processing stopped to prevent a freeze"

        local created = {}
        local permanentNames = {}

        local function track(id)
            -- a nil here would not grow the table, so the trigger would be left
            -- armed for the rest of the suite without anything saying so
            assert(type(id) == "number", "arming a trigger did not return an id")
            created[#created + 1] = id
            return id
        end

        local function trackPermanent(name)
            permanentNames[#permanentNames + 1] = name
            return name
        end

        -- The report is printed onto the line the runaway text is still on, so
        -- the mark is that line rather than the one after it.
        local function consoleMark()
            return getLastLineNumber("main")
        end

        -- The console wraps the report over several buffer lines and eats the
        -- space it broke at, so both sides are matched with all whitespace gone.
        local function consoleSince(mark)
            local last = getLastLineNumber("main")
            -- getLines() answers an out-of-range index with "ERROR: invalid line
            -- number" rather than failing, which would make every scan below
            -- match nothing and pass
            assert.is_true(mark <= last, "the console buffer trimmed past the mark, so this scan would read outside it")
            return (table.concat(getLines("main", mark, last + 1), ""):gsub("%s+", ""))
        end

        -- The report is a tr() string, so the blocks that assert it is absent
        -- stop meaning anything if the wording changes; the one block that
        -- asserts it is present is what fails to say so.
        local function contains(haystack, needle)
            return haystack:find((needle:gsub("%s+", "")), 1, true) ~= nil
        end

        before_each(function()
            created = {}
            permanentNames = {}
            _G.SameLineSpec = {}
        end)

        after_each(function()
            -- a runaway arms a thousand triggers that outlive their line, and
            -- left behind every one of them would be scanned on every line the
            -- rest of the suite feeds
            for _, id in ipairs(created) do
                killTrigger(id)
            end
            -- permanent items have no removal API, so they are switched off
            -- instead; that state is saved, so re-running the suite against the
            -- same profile finds them inert rather than firing. The return is
            -- not checked because a child of a temporary parent goes when its
            -- parent is killed above, and answers "not found" here.
            for _, name in ipairs(permanentNames) do
                disableTrigger(name)
            end
            created = {}
            permanentNames = {}
            _G.SameLineSpec = {}
        end)

        teardown(function()
            _G.SameLineSpec = nil
        end)

        it("lets a temp trigger created in a trigger match the current line", function()
            local captured = {}
            track(tempRegexTrigger("^Room 74042", function()
                track(tempRegexTrigger("^(.*)$", function() captured[#captured + 1] = matches[2] end, 200))
            end))

            feedTriggers("Room 74042: The Bitter Almond Grove\n")
            feedTriggers("Exits: North South West\n")

            assert.are.equal("Room 74042: The Bitter Almond Grove|Exits: North South West", table.concat(captured, "|"),
                "the temp trigger created on the room title line should capture that same line first, then the next line")
        end)

        it("offers a new trigger the current line after the existing ones", function()
            local order = {}
            track(tempRegexTrigger("^o$", function()
                order[#order + 1] = "first"
                track(tempRegexTrigger("^o$", function() order[#order + 1] = "created" end))
            end))
            track(tempRegexTrigger("^o$", function() order[#order + 1] = "second" end))

            feedTriggers("o\n")

            assert.are.equal("first,second,created", table.concat(order, ","),
                "the mid-pass trigger should fire on the current line after all pre-existing triggers")
        end)

        it("lets a chain of creations all match the current line", function()
            local chain = {}
            track(tempRegexTrigger("^c$", function()
                chain[#chain + 1] = "creator"
                track(tempRegexTrigger("^c$", function()
                    chain[#chain + 1] = "A"
                    track(tempRegexTrigger("^c$", function() chain[#chain + 1] = "B" end))
                end))
            end))

            feedTriggers("c\n")

            assert.are.equal("creator,A,B", table.concat(chain, ","),
                "each generation of mid-pass triggers should still match the current line")
        end)

        it("spends a single shot on the line that created it", function()
            local expiryLine = ""
            track(tempRegexTrigger("^e", function()
                track(tempRegexTrigger("^(.*)$", function() expiryLine = expiryLine .. matches[2] .. ";" end, 1))
            end, 1))

            feedTriggers("e one\n")
            feedTriggers("e two\n")

            assert.are.equal("e one;", expiryLine,
                "the single-shot temp trigger should fire once, on the line that created it")
        end)

        it("starts a mid-pass line trigger on the current line", function()
            local lineGrabs = {}
            track(tempRegexTrigger("^lstart$", function()
                track(tempLineTrigger(0, 2, function() lineGrabs[#lineGrabs + 1] = getCurrentLine() end))
            end, 1))

            feedTriggers("lstart\n")
            feedTriggers("second\n")
            feedTriggers("third\n")

            assert.are.equal("lstart,second", table.concat(lineGrabs, ","),
                "the mid-pass line trigger should grab the creating line and the one after it")
        end)

        it("matches both the nested line and the outer one", function()
            local nested = {}
            track(tempRegexTrigger("^outer$", function()
                track(tempRegexTrigger("^(.*)$", function() nested[#nested + 1] = matches[2] end, 10))
                feedTriggers("inner\n")
            end, 1))

            feedTriggers("outer\n")

            assert.are.equal("inner,outer", table.concat(nested, ","),
                "the mid-pass trigger should match the nested line first, then the outer line it was created on")
        end)

        it("leaves a finite creation chain alone", function()
            local fires = 0
            local step
            step = function()
                fires = fires + 1
                if fires < 10 then
                    track(tempRegexTrigger("^chain$", step, 1))
                end
            end
            track(tempRegexTrigger("^chain$", step, 1))

            local mark = consoleMark()
            feedTriggers("chain\n")

            assert.are.equal(10, fires, "all ten generations of the finite chain should match the current line")
            assert.is_false(contains(consoleSince(mark), abortReport),
                "a chain that ends on its own must not trip the same-line generation budget")
        end)

        it("does not stop a batch of unrelated creations", function()
            -- each of them starts a creation chain of its own, and none of those
            -- chains ever gets a second link
            local bulkCount = maxGenerations + 1
            local fires = 0
            track(tempRegexTrigger("^bulkgate$", function()
                for _ = 1, bulkCount do
                    track(tempRegexTrigger("^bulkpay$", function() fires = fires + 1 end))
                end
            end, 1))

            local mark = consoleMark()
            feedTriggers("bulkgate\n")
            feedTriggers("bulkpay\n")

            assert.is_false(contains(consoleSince(mark), abortReport),
                "a batch of unrelated triggers must not be mistaken for a trigger re-creating itself")
            assert.are.equal(bulkCount, fires, "every trigger armed on the previous line should survive and fire")
        end)

        it("keeps both sets when two scripts arm past the budget between them", function()
            -- anything armed from a script that predates the line starts a
            -- lineage of its own, so neither script can exhaust the other's
            local eachCount = (maxGenerations / 2) + 1
            local firesA, firesB = 0, 0
            track(tempRegexTrigger("^sharedgate$", function()
                for _ = 1, eachCount do
                    track(tempRegexTrigger("^payA$", function() firesA = firesA + 1 end))
                end
            end, 1))
            track(tempRegexTrigger("^sharedgate$", function()
                for _ = 1, eachCount do
                    track(tempRegexTrigger("^payB$", function() firesB = firesB + 1 end))
                end
            end, 1))

            feedTriggers("sharedgate\n")
            feedTriggers("payA\n")
            feedTriggers("payB\n")

            assert.are.equal(eachCount, firesA, "the first script should keep every trigger it armed")
            assert.are.equal(eachCount, firesB, "the second script should keep every trigger it armed")
        end)

        it("does not stop a batch armed by a trigger created on the same line", function()
            -- creator and batch share a lineage here, which is the room-capture
            -- shape: the room-title trigger creates the capture trigger, and the
            -- capture trigger is what arms the batch
            local bulkCount = maxGenerations + 1
            local fires = 0
            track(tempRegexTrigger("^deepgate$", function()
                track(tempRegexTrigger("^deepgate$", function()
                    for _ = 1, bulkCount do
                        track(tempRegexTrigger("^deeppay$", function() fires = fires + 1 end))
                    end
                end, 1))
            end, 1))

            local mark = consoleMark()
            feedTriggers("deepgate\n")
            feedTriggers("deeppay\n")

            assert.is_false(contains(consoleSince(mark), abortReport),
                "a batch is one generation wherever it is armed from")
            assert.are.equal(bulkCount, fires,
                "every trigger armed by a trigger created on the same line should survive and fire")
        end)

        it("does not stop a batch of permanent creations", function()
            -- the more painful loss: a "rebuild my triggers when the game says X"
            -- routine arms permanent triggers in bulk
            local bulkCount = maxGenerations + 1
            local fires = 0
            _G.SameLineSpec.permBulkStep = function() fires = fires + 1 end
            _G.SameLineSpec.armPermBulk = function()
                for i = 1, bulkCount do
                    permRegexTrigger(trackPermanent("SpecPermBulk" .. i), "", {"^permpay$"}, [[_G.SameLineSpec.permBulkStep()]])
                end
            end
            track(tempRegexTrigger("^permgate$", function() _G.SameLineSpec.armPermBulk() end, 1))

            feedTriggers("permgate\n")
            feedTriggers("permpay\n")

            assert.are.equal(bulkCount, fires,
                "every permanent trigger armed on the previous line should survive and fire")
            assert.are.equal(1, isActive("SpecPermBulk" .. bulkCount, "trigger"),
                "the permanent triggers should be left switched on")
        end)

        it("leaves a chain of exactly the budget's length alone", function()
            -- the trip is on the generation after this one, which the runaway
            -- block below pins from the other side: both land on the same fire
            -- count, so what tells them apart is whether the lineage was stopped
            local fires = 0
            local armed = {}
            local step
            step = function()
                fires = fires + 1
                if fires <= maxGenerations then
                    armed[#armed + 1] = track(tempRegexTrigger("^boundline$", step, 1))
                end
            end
            armed[#armed + 1] = track(tempRegexTrigger("^boundline$", step, 1))

            local mark = consoleMark()
            feedTriggers("boundline\n")

            assert.is_false(contains(consoleSince(mark), abortReport),
                "a chain of exactly the budget's length ends on its own and must not be stopped")
            assert.are.equal(maxGenerations + 1, fires, "the chain should run to its own end")
            assert.are.equal(maxGenerations + 1, #armed, "the chain should stop arming of its own accord")
        end)

        it("starts fresh chains for a trigger that outlived its line", function()
            -- otherwise it would carry its creator's chain around for the rest of
            -- the session
            local bulkCount = maxGenerations + 1
            local fires = 0
            track(tempRegexTrigger("^egate$", function()
                track(tempRegexTrigger("^esecond$", function()
                    for _ = 1, bulkCount do
                        track(tempRegexTrigger("^epay$", function() fires = fires + 1 end))
                    end
                end, 1))
            end, 1))

            local mark = consoleMark()
            feedTriggers("egate\n")
            feedTriggers("esecond\n")
            feedTriggers("epay\n")

            assert.is_false(contains(consoleSince(mark), abortReport),
                "a trigger created on an earlier line is not part of a chain any more and must arm freely")
            assert.are.equal(bulkCount, fires, "every trigger armed on the later line should survive and fire")
        end)

        it("does not stop a batch armed inside a nested pass", function()
            -- creations made inside a nested pass are appended to the same list
            -- the outer pass is walking, so a batch armed there is one generation
            -- just the same
            local bulkCount = maxGenerations + 1
            local fires = 0
            track(tempRegexTrigger("^crossout$", function()
                track(tempRegexTrigger("^crossin$", function()
                    for _ = 1, bulkCount do
                        track(tempRegexTrigger("^crosspay$", function() fires = fires + 1 end))
                    end
                end, 1))
                feedTriggers("crossin\n")
            end, 1))

            local mark = consoleMark()
            feedTriggers("crossout\n")
            feedTriggers("crosspay\n")

            assert.is_false(contains(consoleSince(mark), abortReport),
                "a batch armed inside a nested pass is still one generation")
            assert.are.equal(bulkCount, fires,
                "every trigger armed inside the nested pass should survive and fire")
        end)

        local reportedAt

        -- The first runaway of the run, so the report is not throttled yet and
        -- this is the one block that can read it.
        it("names the trigger in the abort report", function()
            local armNamed
            armNamed = function()
                track(tempComplexRegexTrigger("hpWatcher", "^hpnamed$", armNamed, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1))
            end
            armNamed()

            local mark = consoleMark()
            feedTriggers("hpnamed\n")
            local reported = consoleSince(mark)

            reportedAt = os.time()
            assert.is_true(contains(reported, abortReport), "the abort should be reported to the console")
            assert.is_true(contains(reported, "trigger 'hpWatcher'"),
                "the report should name the trigger that keeps re-creating itself")
        end)

        -- Immediately after the block above, so the ten-second window it opened
        -- is still shut. Everything below depends on that window, and this is
        -- where it is checked rather than assumed.
        it("reports a second runaway within the throttle window only once", function()
            local fires = 0
            local arm
            arm = function()
                track(tempRegexTrigger("^throttled$", function()
                    fires = fires + 1
                    arm()
                end, 1))
            end
            arm()

            local mark = consoleMark()
            feedTriggers("throttled\n")

            assert.are.equal(1 + maxGenerations, fires, "the second runaway should be stopped just the same")
            -- a runner slow enough to have spent the window between the two
            -- blocks has nothing to say about the throttle, rather than a
            -- wall-clock failure to report
            if reportedAt and os.time() - reportedAt < 9 then
                assert.is_false(contains(consoleSince(mark), abortReport),
                    "a runaway whose creator outlives the line trips on every matching line, and repeating the report would bury the game text")
            end
        end)

        it("stops a trigger that re-creates itself", function()
            -- the naive "one-shot that re-arms itself at the end of its own
            -- handler" is the shape users write; without the budget it hangs
            local fires = 0
            local armed = {}
            local arm
            arm = function()
                armed[#armed + 1] = track(tempRegexTrigger("^hploop$", function()
                    fires = fires + 1
                    arm()
                end, 1))
            end
            arm()

            feedTriggers("hploop\n")

            assert.are.equal(1 + maxGenerations, fires,
                "one fire from the trigger already there, then one per budgeted creation")
            assert.are.equal(2 + maxGenerations, #armed, "the generation over the budget is created before it is refused")
            assert.are.equal(0, isActive(armed[#armed], "trigger"),
                "the generation that tripped the budget should have been stopped, not left armed")
        end)

        it("charges each line its own budget", function()
            -- without disowning what the loop created, each line would cost a
            -- multiple of the one before it and the freeze would only be postponed
            local fires = 0
            local arm
            arm = function()
                track(tempRegexTrigger("^kept$", function()
                    fires = fires + 1
                    arm()
                end))
            end
            arm()

            feedTriggers("kept\n")
            feedTriggers("kept\n")

            assert.are.equal(2 * (1 + maxGenerations), fires,
                "the second line should cost the same as the first, not a multiple of it")
        end)

        it("stops a permanent trigger that re-creates itself without deleting it", function()
            -- permanent triggers are saved with the profile, so they are stopped
            -- with deactivate(), which leaves the user-active state XMLexport
            -- writes alone
            local name = trackPermanent("SpecSameLinePermLoop")
            local fires = 0
            _G.SameLineSpec.armPerm = function()
                permRegexTrigger(name, "", {"^permloop$"}, [[_G.SameLineSpec.permStep()]])
            end
            _G.SameLineSpec.permStep = function()
                fires = fires + 1
                _G.SameLineSpec.armPerm()
            end
            local existedBefore = exists(name, "trigger")
            local activeBefore = isActive(name, "trigger")
            _G.SameLineSpec.armPerm()

            feedTriggers("permloop\n")

            assert.are.equal(1 + maxGenerations, fires, "the re-arming permanent trigger should fire once per budgeted creation")
            assert.are.equal(activeBefore + 1, isActive(name, "trigger"),
                "only the trigger that predates the line should still be active")
            assert.are.equal(existedBefore + 2 + maxGenerations, exists(name, "trigger"),
                "the stopped permanent triggers should still exist - stopping them is not deleting them")
        end)

        it("spares triggers another script armed on the same line", function()
            local innocentFires = 0
            local runaway = {}
            local armRunaway
            armRunaway = function()
                runaway[#runaway + 1] = track(tempRegexTrigger("^runline$", armRunaway, 1))
            end
            armRunaway()
            track(tempRegexTrigger("^runline$", function()
                track(tempRegexTrigger("^innocent$", function() innocentFires = innocentFires + 1 end))
            end, 1))

            feedTriggers("runline\n")
            feedTriggers("innocent\n")

            -- pin the chain length first: isActive() answers 0 for an id that no
            -- longer exists too, so on its own it would also be happy with a
            -- chain that never got past its first link
            assert.are.equal(2 + maxGenerations, #runaway, "the runaway should have re-created itself once per budgeted generation")
            assert.are.equal(0, isActive(runaway[#runaway], "trigger"), "the self-recreating chain should still be stopped")
            assert.are.equal(1, innocentFires,
                "the trigger armed by an unrelated script on the same line should survive the runaway's abort and fire")
        end)

        it("stops a runaway that crosses into a nested pass whole", function()
            -- the lineage has members either side of the nested pass's first-node
            -- index, so stopping it scans the whole list rather than the tail of
            -- the tripping pass; scanning only the tail leaves the first link
            -- alive and the outer pass trips on the same lineage all over again
            local nestFires, nestSafeFires = 0, 0
            local armNested
            armNested = function()
                track(tempRegexTrigger("^nestin$", function()
                    nestFires = nestFires + 1
                    armNested()
                end))
            end
            track(tempRegexTrigger("^nestout$", function()
                armNested()
                track(tempRegexTrigger("^nestsafe$", function() nestSafeFires = nestSafeFires + 1 end))
                feedTriggers("nestin\n")
            end, 1))

            feedTriggers("nestout\n")
            local firesInsideTheNestedPass = nestFires
            nestFires = 0
            feedTriggers("nestin\n")
            feedTriggers("nestsafe\n")

            assert.are.equal(maxGenerations, firesInsideTheNestedPass,
                "the runaway should cost one budget, not one per pass the lineage is spread across")
            assert.are.equal(0, nestFires, "no member of the stopped lineage should be left armed, wherever in the list it sat")
            assert.are.equal(1, nestSafeFires,
                "a trigger armed by an unrelated script on the outer line should survive the nested pass's abort")
        end)

        it("counts a folder child's creations against its root", function()
            -- only root triggers carry a lineage, so a trigger sitting in a folder
            -- creates on the folder's behalf. Reading the child's own (always
            -- empty) lineage instead would start a fresh one every round, which
            -- never deepens and so never trips.
            local fires, generation = 0, 0
            _G.SameLineSpec.makeFolderGen = function()
                generation = generation + 1
                local folder = trackPermanent("SpecFGen" .. generation)
                permGroup(folder, "trigger")
                permRegexTrigger(trackPermanent("SpecFChild" .. generation), folder, {"^folderloop$"}, [[_G.SameLineSpec.folderStep()]])
            end
            _G.SameLineSpec.folderStep = function()
                fires = fires + 1
                _G.SameLineSpec.makeFolderGen()
            end
            _G.SameLineSpec.makeFolderGen()

            feedTriggers("folderloop\n")

            assert.are.equal(1 + maxGenerations, fires,
                "the folder's lineage should deepen by one per round, so the generation budget is what ends it")
            assert.are.equal(2 + maxGenerations, generation, "the generation over the budget is created before it is refused")
            assert.is_true(exists("SpecFGen" .. generation, "trigger") >= 1, "the last folder should have been created")
            assert.are.equal(0, isActive("SpecFGen" .. generation, "trigger"),
                "the folder that tripped the budget should have been switched off, not left armed")
        end)

        it("counts a filter chain child's creations against its root", function()
            -- the same for a filter chain, where the child is reached through the
            -- parent's capture rather than by the root list passing data down
            local fires, generation = 0, 0
            _G.SameLineSpec.makeFilterGen = function()
                generation = generation + 1
                local parent = "SpecFiltP" .. generation
                track(tempComplexRegexTrigger(parent, "^(filterloop)$", "", 0, 0, 0, 1, 0, 0, 0, 0, 0, 0))
                permRegexTrigger(trackPermanent("SpecFiltC" .. generation), parent, {"filterloop"}, [[_G.SameLineSpec.filterStep()]])
            end
            _G.SameLineSpec.filterStep = function()
                fires = fires + 1
                _G.SameLineSpec.makeFilterGen()
            end
            _G.SameLineSpec.makeFilterGen()

            feedTriggers("filterloop\n")

            assert.are.equal(1 + maxGenerations, fires,
                "the filter parent's lineage should deepen by one per round, so the generation budget is what ends it")
            assert.are.equal(2 + maxGenerations, generation, "the generation over the budget is created before it is refused")
            assert.are.equal(0, exists("SpecFiltP" .. generation, "trigger"),
                "the temporary filter parent that tripped the budget should have been removed")
        end)

        it("leaves the outer line's own triggers alone when a nested pass aborts", function()
            local seen = {}
            local inner = {}
            local armInner
            armInner = function()
                inner[#inner + 1] = track(tempRegexTrigger("^inner$", armInner, 1))
            end
            armInner()
            track(tempRegexTrigger("^outer$", function()
                track(tempRegexTrigger("^(.*)$", function() seen[#seen + 1] = matches[2] end, 10))
                feedTriggers("inner\n")
            end, 1))

            feedTriggers("outer\n")

            assert.are.equal(2 + maxGenerations, #inner, "the runaway should have re-created itself once per budgeted generation")
            assert.are.equal(0, isActive(inner[#inner], "trigger"), "the runaway in the nested pass should be stopped")
            assert.are.equal("inner,outer", table.concat(seen, ","),
                "the capture trigger created by the outer line should survive the nested pass's abort and still match the outer line")
        end)

        it("stops a runaway driven by server text", function()
            -- not a feedTriggers() curiosity: server text takes the same path
            local fires = 0
            local armed = {}
            local arm
            arm = function()
                armed[#armed + 1] = track(tempRegexTrigger("^HP: 100/100$", function()
                    fires = fires + 1
                    arm()
                end, 1))
            end
            arm()

            local ok, message = feedTelnet("HP: 100/100\r\n")
            assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(message))

            assert.are.equal(1 + maxGenerations, fires, "server text should reach the same-line generation budget")
            assert.are.equal(0, isActive(armed[#armed], "trigger"),
                "the generation that tripped the budget should have been stopped, not left armed")
        end)

    end)
end)
