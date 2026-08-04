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

    -- enableAlias()/disableAlias() must toggle EVERY alias sharing a name, not
    -- just the first, since AliasUnit iterates the whole multimap of same-named
    -- entries.
    describe("enable/disable with duplicate names", function()

        before_each(function()
            _G.DuplicateAliasTest = {fired = {}}
        end)

        -- permAlias aliases can't be removed via killAlias(); disable them here so
        -- a failed assertion above doesn't leave them firing for later tests
        after_each(function()
            disableAlias("Druid Aliases")
            disableAlias("Other Aliases")
        end)

        it("toggles every alias sharing the same name, not just the first", function()
            -- permanent so two can share one name; distinct patterns reveal which fired
            local id1 = permAlias("Druid Aliases", "", "^druid_dup_one$", [[_G.DuplicateAliasTest.fired.one = true]])
            local id2 = permAlias("Druid Aliases", "", "^druid_dup_two$", [[_G.DuplicateAliasTest.fired.two = true]])
            assert.is_true(id1 > 0, "first duplicate-named alias should be created")
            assert.is_true(id2 > 0, "second duplicate-named alias should be created")

            expandAlias("druid_dup_one")
            expandAlias("druid_dup_two")
            assert.is_true(_G.DuplicateAliasTest.fired.one, "alias 1 should fire while enabled")
            assert.is_true(_G.DuplicateAliasTest.fired.two, "alias 2 should fire while enabled")

            _G.DuplicateAliasTest.fired = {}
            disableAlias("Druid Aliases")
            expandAlias("druid_dup_one")
            expandAlias("druid_dup_two")
            assert.is_nil(_G.DuplicateAliasTest.fired.one, "alias 1 should be disabled")
            assert.is_nil(_G.DuplicateAliasTest.fired.two, "alias 2 (the second duplicate) should ALSO be disabled")

            _G.DuplicateAliasTest.fired = {}
            enableAlias("Druid Aliases")
            expandAlias("druid_dup_one")
            expandAlias("druid_dup_two")
            assert.is_true(_G.DuplicateAliasTest.fired.one, "alias 1 should be re-enabled")
            assert.is_true(_G.DuplicateAliasTest.fired.two, "alias 2 (the second duplicate) should ALSO be re-enabled")

            -- a differently-named alias must stay untouched
            local idOther = permAlias("Other Aliases", "", "^druid_dup_other$", [[_G.DuplicateAliasTest.fired.other = true]])
            assert.is_true(idOther > 0)
            disableAlias("Druid Aliases")
            _G.DuplicateAliasTest.fired = {}
            expandAlias("druid_dup_other")
            assert.is_true(_G.DuplicateAliasTest.fired.other, "differently-named alias must stay enabled")
        end)

    end)

    describe("tempAlias creation and firing", function()

        after_each(function()
            _G.AliasSpec = nil
        end)

        it("fires a string-body alias created from a regex pattern", function()
            _G.AliasSpec = {fired = false}
            local id = tempAlias("^spec_temp_alias$", [[_G.AliasSpec.fired = true]])
            assert.is_number(id)
            assert.are.equal(1, exists(id, "alias"), "the temp alias should exist after creation")
            expandAlias("spec_temp_alias")
            local fired = _G.AliasSpec.fired
            killAlias(id)
            assert.is_true(fired, "a string-body temp alias should fire on a matching command")
        end)

        it("fires a function-callback alias", function()
            _G.AliasSpec = {fired = false}
            local id = tempAlias("^spec_temp_alias_fn$", function() _G.AliasSpec.fired = true end)
            assert.is_number(id)
            expandAlias("spec_temp_alias_fn")
            local fired = _G.AliasSpec.fired
            killAlias(id)
            assert.is_true(fired, "a function-callback temp alias should fire on a matching command")
        end)

        it("rejects a non-string, non-function body", function()
            -- a table is not string-coercible (a number would be accepted as code)
            assert.has_error(function() tempAlias("^bad$", {}) end)
        end)

    end)

    describe("permAlias argument validation", function()

        it("errors when the lua code (argument 4) is not a string", function()
            local ok, err = pcall(function()
                permAlias("SpecPermAliasBad", "", "^whatever$", 999)
            end)
            assert.is_false(ok, "invalid lua code should error")
            assert.is_truthy(tostring(err):find("permAlias", 1, true),
                "the error should name permAlias, got: " .. tostring(err))
        end)

        it("errors when the regex pattern (argument 3) is missing", function()
            assert.has_error(function()
                permAlias("SpecPermAliasBad", "")
            end)
        end)

        it("errors when the alias name (argument 1) is missing", function()
            assert.has_error(function()
                permAlias()
            end)
        end)

    end)

    describe("killAlias, enable/disable and isActive for aliases", function()

        after_each(function()
            -- perm aliases can't be killed; disable any created above so they
            -- don't leak into other specs
            disableAlias("SpecPermAliasKill")
            disableAlias("SpecPermAliasToggle")
            _G.AliasSpec = nil
        end)

        it("killAlias returns true for a temporary alias and false for a missing one", function()
            local id = tempAlias("^spec_kill_alias$", [[]])
            assert.are.equal(1, exists(id, "alias"))
            assert.is_true(killAlias(id), "killing an existing temp alias should return true")
            assert.is_false(killAlias("no_such_alias_name"), "killing a missing alias should return false")
        end)

        it("killAlias returns false the second time, as the alias is already dead", function()
            local id = tempAlias("^spec_double_kill_alias$", [[]])
            assert.is_true(killAlias(id), "killing a live temporary alias should report success")
            -- the alias is still present here: only the deferred cleanup frees it, so
            -- the second kill really is being told about a corpse it can find
            assert.are.equal(1, exists(id, "alias"), "the killed alias is still present until cleanup runs")
            assert.are.equal(0, isActive(id, "alias"), "a killed alias is no longer active")
            assert.is_false(killAlias(id),
                "killing an already killed alias achieves nothing and has to say so")
            -- an incoming line runs every unit's deferred cleanup, which is what
            -- finally frees the alias; the answer has to be the same after it
            feedTriggers("\nspec_alias_kill_flush\n")
            assert.are.equal(0, exists(id, "alias"), "the alias should be gone after kill and cleanup")
            assert.is_false(killAlias(id), "a freed alias cannot be killed either")
        end)

        it("killAlias returns false the second time inside the alias's own script", function()
            _G.AliasSpec = {}
            local id
            id = tempAlias("^spec_self_kill_alias$", function()
                _G.AliasSpec.killed = killAlias(id)
                _G.AliasSpec.killedAgain = killAlias(id)
            end)
            expandAlias("spec_self_kill_alias")
            assert.is_not_nil(_G.AliasSpec.killed, "the alias should have matched and run")
            assert.is_true(_G.AliasSpec.killed,
                "killAlias should report success from inside the alias's own script")
            assert.is_false(_G.AliasSpec.killedAgain,
                "killing the same alias twice from its own script must fail the second time")
        end)

        it("killAlias returns false for a permanent alias (they cannot be killed)", function()
            local id = permAlias("SpecPermAliasKill", "", "^spec_perm_kill$", [[]])
            assert.is_true(id > 0)
            assert.is_false(killAlias("SpecPermAliasKill"), "permanent aliases cannot be removed with killAlias")
        end)

        it("enableAlias and disableAlias return whether a matching alias was found", function()
            local id = permAlias("SpecPermAliasToggle", "", "^spec_toggle_alias$", [[]])
            assert.is_true(id > 0)
            assert.is_true(disableAlias("SpecPermAliasToggle"), "disableAlias should report it found the alias")
            assert.is_true(enableAlias("SpecPermAliasToggle"), "enableAlias should report it found the alias")
            assert.is_false(disableAlias("no_such_alias_name"), "disableAlias should return false when nothing matched")
            assert.is_false(enableAlias("no_such_alias_name"), "enableAlias should return false when nothing matched")
        end)

        it("isActive reflects an alias's enabled state", function()
            local id = tempAlias("^spec_active_alias$", [[]])
            assert.are.equal(1, isActive(id, "alias"), "a fresh alias should be active")
            disableAlias(id)
            assert.are.equal(0, isActive(id, "alias"), "a disabled alias should not be active")
            enableAlias(id)
            assert.are.equal(1, isActive(id, "alias"), "a re-enabled alias should be active")
            killAlias(id)
        end)

    end)
end)
