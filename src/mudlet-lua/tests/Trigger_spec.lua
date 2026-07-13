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
end)
