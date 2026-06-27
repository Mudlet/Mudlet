describe("Alias processing", function()

    -- Test for nested alias processing with self-deletion (GitHub issue #8817)
    -- This verifies the fix that uses mProcessingDepth counter instead of a bool flag
    --
    -- The original bug: when using a bool mIsProcessing flag, nested alias calls
    -- via expandAlias() would set the flag to false when the inner call completed,
    -- causing doCleanup() to run while the outer alias was still processing.
    -- This led to crashes when the outer alias tried to access deleted objects.
    --
    -- The fix uses an integer mProcessingDepth counter that increments on entry
    -- and decrements on exit, only running cleanup when it reaches zero.
    describe("nested processing", function()

        it("should not crash when inner alias kills itself during nested expandAlias", function()
            local inner_id
            local outer_executed = false
            local inner_executed = false

            local outer_id = tempAlias("^test_outer$", function()
                outer_executed = true
                expandAlias("test_inner")
            end)

            inner_id = tempAlias("^test_inner$", function()
                inner_executed = true
                killAlias(inner_id)
            end)

            -- Verify both aliases exist before the test
            assert.are.equal(1, exists(outer_id, "alias"), "Outer alias should exist before test")
            assert.are.equal(1, exists(inner_id, "alias"), "Inner alias should exist before test")

            -- This should not crash - the fix defers cleanup until all processing completes
            -- Without the fix, this would crash because:
            -- 1. outer alias matches, mProcessingDepth becomes 1
            -- 2. inner alias matches (nested), mProcessingDepth becomes 2
            -- 3. inner alias calls killAlias() -> markCleanup()
            -- 4. inner processing ends, mProcessingDepth becomes 1, no cleanup (depth > 0)
            -- 5. outer processing ends, mProcessingDepth becomes 0, cleanup runs safely
            expandAlias("test_outer")

            assert.is_true(outer_executed, "Outer alias should have executed")
            assert.is_true(inner_executed, "Inner alias should have executed")

            -- Verify cleanup ran correctly: inner alias should be deleted
            assert.are.equal(0, exists(inner_id, "alias"), "Inner alias should have been cleaned up")

            -- Verify outer alias still exists (wasn't incorrectly deleted)
            assert.are.equal(1, exists(outer_id, "alias"), "Outer alias should still exist")

            -- Cleanup
            killAlias(outer_id)
        end)

        it("should handle double-nested alias processing with cleanup", function()
            local level3_id
            local executions = {}

            local level1_id = tempAlias("^test_level1$", function()
                table.insert(executions, "level1_start")
                expandAlias("test_level2")
                table.insert(executions, "level1_end")
            end)

            local level2_id = tempAlias("^test_level2$", function()
                table.insert(executions, "level2_start")
                expandAlias("test_level3")
                table.insert(executions, "level2_end")
            end)

            level3_id = tempAlias("^test_level3$", function()
                table.insert(executions, "level3")
                killAlias(level3_id)
            end)

            -- Verify all aliases exist before the test
            assert.are.equal(1, exists(level1_id, "alias"), "Level 1 alias should exist")
            assert.are.equal(1, exists(level2_id, "alias"), "Level 2 alias should exist")
            assert.are.equal(1, exists(level3_id, "alias"), "Level 3 alias should exist")

            -- This tests 3 levels of nesting with cleanup at the deepest level
            -- mProcessingDepth goes: 0 -> 1 -> 2 -> 3 -> 2 -> 1 -> 0, then cleanup
            expandAlias("test_level1")

            -- Verify all levels executed in correct order (depth-first)
            assert.are.equal(5, #executions, "All execution points should have been reached")
            assert.are.equal("level1_start", executions[1])
            assert.are.equal("level2_start", executions[2])
            assert.are.equal("level3", executions[3])
            assert.are.equal("level2_end", executions[4])
            assert.are.equal("level1_end", executions[5])

            -- Verify level3 was cleaned up, others still exist
            assert.are.equal(0, exists(level3_id, "alias"), "Level 3 alias should have been cleaned up")
            assert.are.equal(1, exists(level1_id, "alias"), "Level 1 alias should still exist")
            assert.are.equal(1, exists(level2_id, "alias"), "Level 2 alias should still exist")

            -- Cleanup
            killAlias(level1_id)
            killAlias(level2_id)
        end)

        it("should handle multiple aliases being killed in nested processing", function()
            local inner1_id, inner2_id
            local execution_order = {}

            local outer_id = tempAlias("^test_multi_outer$", function()
                table.insert(execution_order, "outer_start")
                expandAlias("test_multi_inner1")
                expandAlias("test_multi_inner2")
                table.insert(execution_order, "outer_end")
            end)

            inner1_id = tempAlias("^test_multi_inner1$", function()
                table.insert(execution_order, "inner1")
                killAlias(inner1_id)
            end)

            inner2_id = tempAlias("^test_multi_inner2$", function()
                table.insert(execution_order, "inner2")
                killAlias(inner2_id)
            end)

            expandAlias("test_multi_outer")

            -- Verify execution order
            assert.are.equal(4, #execution_order)
            assert.are.equal("outer_start", execution_order[1])
            assert.are.equal("inner1", execution_order[2])
            assert.are.equal("inner2", execution_order[3])
            assert.are.equal("outer_end", execution_order[4])

            -- Both inner aliases should be cleaned up
            assert.are.equal(0, exists(inner1_id, "alias"), "Inner1 should be cleaned up")
            assert.are.equal(0, exists(inner2_id, "alias"), "Inner2 should be cleaned up")
            assert.are.equal(1, exists(outer_id, "alias"), "Outer should still exist")

            killAlias(outer_id)
        end)

    end)

    -- Reproduces a user report that enableAlias()/disableAlias() only affect the
    -- FIRST item sharing a given name when several aliases share that name (e.g.
    -- two alias groups both called "Druid Aliases"). AliasUnit::enableAlias and
    -- AliasUnit::disableAlias iterate the whole multimap of same-named entries,
    -- so EVERY alias with that name must be toggled, not just the first found.
    --
    -- killAlias() only removes temporary aliases, and there is no Lua API to
    -- delete permanent ones, so these aliases are given a unique name and left
    -- disabled at the end to avoid interfering with other specs.
    describe("enable/disable with duplicate names", function()

        before_each(function()
            _G.DuplicateAliasTest = {fired = {}}
        end)

        it("toggles every alias sharing the same name, not just the first", function()
            -- permanent aliases so two of them can share one name; distinct
            -- patterns let us tell which one actually fired
            local id1 = permAlias("Druid Aliases", "", "^druid_dup_one$", [[_G.DuplicateAliasTest.fired.one = true]])
            local id2 = permAlias("Druid Aliases", "", "^druid_dup_two$", [[_G.DuplicateAliasTest.fired.two = true]])
            assert.is_true(id1 > 0, "first duplicate-named alias should be created")
            assert.is_true(id2 > 0, "second duplicate-named alias should be created")

            -- sanity: permAlias creates them active, so both fire
            expandAlias("druid_dup_one")
            expandAlias("druid_dup_two")
            assert.is_true(_G.DuplicateAliasTest.fired.one, "alias 1 should fire while enabled")
            assert.is_true(_G.DuplicateAliasTest.fired.two, "alias 2 should fire while enabled")

            -- disableAlias must disable BOTH, not only the first one found
            _G.DuplicateAliasTest.fired = {}
            disableAlias("Druid Aliases")
            expandAlias("druid_dup_one")
            expandAlias("druid_dup_two")
            assert.is_nil(_G.DuplicateAliasTest.fired.one, "alias 1 should be disabled")
            assert.is_nil(_G.DuplicateAliasTest.fired.two, "alias 2 (the second duplicate) should ALSO be disabled")

            -- enableAlias must re-enable BOTH
            _G.DuplicateAliasTest.fired = {}
            enableAlias("Druid Aliases")
            expandAlias("druid_dup_one")
            expandAlias("druid_dup_two")
            assert.is_true(_G.DuplicateAliasTest.fired.one, "alias 1 should be re-enabled")
            assert.is_true(_G.DuplicateAliasTest.fired.two, "alias 2 (the second duplicate) should ALSO be re-enabled")

            -- aliases with a DIFFERENT name must be unaffected (control)
            local idOther = permAlias("Other Aliases", "", "^druid_dup_other$", [[_G.DuplicateAliasTest.fired.other = true]])
            assert.is_true(idOther > 0)
            disableAlias("Druid Aliases")
            _G.DuplicateAliasTest.fired = {}
            expandAlias("druid_dup_other")
            assert.is_true(_G.DuplicateAliasTest.fired.other, "differently-named alias must stay enabled")

            -- no Lua API removes permanent aliases; leave them disabled
            disableAlias("Druid Aliases")
            disableAlias("Other Aliases")
        end)

    end)
end)
