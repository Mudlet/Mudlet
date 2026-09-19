describe("Alias processing", function()

    -- TAlias's match-all loop is unconditional, and it steps one byte after an
    -- empty match, so on a command holding a multi-byte character it can land
    -- mid-character. pcre2 then rejects the offset and TAlias::match() ends the
    -- loop, dropping every capture past that character.
    describe("captures across a multi-byte character", function()

        it("keeps collecting captures past a multi-byte character", function()
            -- expandAlias() sends the command through the same encoding path as
            -- typing it, so a non-UTF-8 encoding would strip the character and let
            -- this pass without testing anything
            assert.are.equal("UTF-8", getServerEncoding(), "this spec needs a UTF-8 server encoding to send a multi-byte command")
            local seen = {}
            local id = tempAlias([[(\d*)]], function()
                seen = {}
                for i = 1, #matches do
                    seen[i] = matches[i]
                end
            end)
            expandAlias("caf\195\169 9", false)
            assert.is_true(killAlias(id), "a temporary alias should be removable by id")
            local found = false
            for _, capture in ipairs(seen) do
                if capture == "9" then
                    found = true
                end
            end
            assert.is_true(found, "the capture after the multi-byte character was dropped")
        end)

    end)

    describe("runaway recursion", function()

        it("stops an alias that keeps expanding into itself", function()
            local fired = 0
            local id = tempAlias("^expand_into_myself$", function()
                fired = fired + 1
                expandAlias("expand_into_myself", false)
            end)
            local sends = 0
            local handler = registerAnonymousEventHandler("sysDataSendRequest", function(_, command)
                if command == "expand_into_myself" then
                    sends = sends + 1
                end
            end)

            expandAlias("expand_into_myself", false)

            killAnonymousEventHandler(handler)
            assert.is_true(killAlias(id), "a temporary alias should be removable by id")
            -- One run per level; the call past the cap is refused instead of matched
            assert.are.equal(50, fired)
            assert.are.equal(1, sends, "the call past the cap should go to the game once, unexpanded")
        end)

        -- The "command" field of an alias is sent as if typed, so one that
        -- matches its own pattern recurses without any Lua in between
        it("stops an alias whose command matches itself", function()
            if not os.getenv("MUDLET_TEST_MODE") then
                pending("uninstalling the fixture needs pumpEvents(), which does nothing outside MUDLET_TEST_MODE")
                return
            end
            local path = getMudletHomeDir() .. "/alias-runaway-command.xml"
            finally(function()
                -- uninstallPackage() refuses while the profile save the install
                -- started is still running
                local removed = false
                for _ = 1, 100 do
                    if uninstallPackage("alias-runaway-command") == true then
                        removed = true
                        break
                    end
                    pumpEvents(50)
                end
                os.remove(path)
                -- let the save the uninstall queues run now rather than during
                -- the next spec
                pumpEvents(200)
                assert.is_true(removed, "could not uninstall the runaway alias package")
            end)
            local file = assert(io.open(path, "w"))
            file:write([[<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
	<TriggerPackage />
	<TimerPackage />
	<AliasPackage>
		<Alias isActive="yes" isFolder="no">
			<name>runaway_command_alias</name>
			<script></script>
			<command>runaway_command</command>
			<packageName></packageName>
			<regex>^runaway_command$</regex>
		</Alias>
	</AliasPackage>
	<ActionPackage />
	<ScriptPackage />
	<KeyPackage />
	<VariablePackage>
		<HiddenVariables />
	</VariablePackage>
</MudletPackage>
]])
            file:close()
            assert.is_true(installPackage(path))

            local mark = getLastLineNumber("main")
            expandAlias("runaway_command", false)

            local stopped = false
            for _, line in ipairs(getLines("main", mark, getLastLineNumber("main") + 1)) do
                if line:find('Alias processing stopped to prevent a crash: "runaway_command"', 1, true) then
                    stopped = true
                end
            end
            assert.is_true(stopped, "the runaway alias was not reported on the main console")
        end)

    end)

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

    -- A nested expandAlias() runs a whole alias pass inside the caller's script,
    -- and that pass sets the "command" global and the capture groups for itself.
    -- Whatever ran the outer script has to get its own state back when the
    -- nested call returns, or every capture it reads afterwards is nil.
    describe("state across a nested expandAlias", function()

        it("gives the outer alias back its matches and command", function()
            local seen = {}
            local innerId = tempAlias([[^nested_state_inner (\w+)$]], function()
                seen.innerCommand = command
                seen.innerMatch = matches[2]
            end)
            local outerId = tempAlias([[^nested_state_outer (\w+)$]], function()
                seen.beforeCommand = command
                seen.beforeMatch = matches[2]
                expandAlias("nested_state_inner deeper", false)
                seen.afterCommand = command
                seen.afterFullMatch = matches[1]
                seen.afterMatch = matches[2]
            end)
            finally(function()
                killAlias(innerId)
                killAlias(outerId)
            end)

            expandAlias("nested_state_outer thing", false)

            assert.are.equal("nested_state_outer thing", seen.beforeCommand)
            assert.are.equal("thing", seen.beforeMatch)
            assert.are.equal("nested_state_inner deeper", seen.innerCommand, "the inner alias should see its own command")
            assert.are.equal("deeper", seen.innerMatch, "the inner alias should see its own captures")
            assert.are.equal("nested_state_outer thing", seen.afterCommand, "command was left holding the nested command")
            assert.are.equal("nested_state_outer thing", seen.afterFullMatch, "matches[1] was emptied by the nested expansion")
            assert.are.equal("thing", seen.afterMatch, "the outer capture was emptied by the nested expansion")
        end)

        -- The capture machinery is shared with triggers, so a trigger script that
        -- expands an alias loses its captures the same way
        it("gives the calling trigger back its matches", function()
            local seen = {}
            local aliasId = tempAlias([[^nested_state_from_trigger (\w+)$]], function()
                seen.aliasMatch = matches[2]
            end)
            local triggerId = tempRegexTrigger([[^nested_state_trigger (\w+)$]], function()
                seen.beforeMatch = matches[2]
                expandAlias("nested_state_from_trigger deeper", false)
                seen.afterFullMatch = matches[1]
                seen.afterMatch = matches[2]
            end)
            finally(function()
                killAlias(aliasId)
                killTrigger(triggerId)
            end)

            feedTriggers("\nnested_state_trigger thing\n")

            assert.are.equal("thing", seen.beforeMatch, "the trigger should see its own capture")
            assert.are.equal("deeper", seen.aliasMatch, "the alias should see its own capture")
            assert.are.equal("nested_state_trigger thing", seen.afterFullMatch, "matches[1] was emptied by the nested expansion")
            assert.are.equal("thing", seen.afterMatch, "the trigger's capture was emptied by the nested expansion")
        end)

        -- "command" is set before any pattern is tried, so the commonest shape of
        -- all - expandAlias() used to push a plain command at the game - moves it
        -- even though nothing matches
        it("leaves the caller's command alone when the nested command matches nothing", function()
            local seen = {}
            local outerId = tempAlias([[^unmatched_outer (\w+)$]], function()
                expandAlias("unmatched_by_any_alias_at_all", false)
                seen.command = command
                seen.match = matches[2]
            end)
            finally(function()
                killAlias(outerId)
            end)

            expandAlias("unmatched_outer thing", false)

            assert.are.equal("unmatched_outer thing", seen.command, "command was left holding a command no alias even matched")
            assert.are.equal("thing", seen.match)
        end)

        it("hands every level of a three deep nesting its own state back", function()
            local seen = {}
            local thirdId = tempAlias([[^depth_three (\w+)$]], function()
                seen.third = matches[2]
                seen.thirdCommand = command
            end)
            local secondId = tempAlias([[^depth_two (\w+)$]], function()
                expandAlias("depth_three ccc", false)
                seen.second = matches[2]
                seen.secondCommand = command
            end)
            local firstId = tempAlias([[^depth_one (\w+)$]], function()
                expandAlias("depth_two bbb", false)
                seen.first = matches[2]
                seen.firstCommand = command
            end)
            finally(function()
                killAlias(firstId)
                killAlias(secondId)
                killAlias(thirdId)
            end)

            expandAlias("depth_one aaa", false)

            assert.are.equal("ccc", seen.third)
            assert.are.equal("depth_three ccc", seen.thirdCommand)
            assert.are.equal("bbb", seen.second, "the middle level got another level's captures back")
            assert.are.equal("depth_two bbb", seen.secondCommand, "the middle level got another level's command back")
            assert.are.equal("aaa", seen.first, "the outermost level got another level's captures back")
            assert.are.equal("depth_one aaa", seen.firstCommand, "the outermost level got another level's command back")
        end)

        -- Handing back a table rebuilt from the capture list would lose whatever
        -- the script had put in the one it was actually given
        it("gives back the very matches table the caller was holding", function()
            local seen = {}
            local innerId = tempAlias([[^own_table_inner$]], function() end)
            local outerId = tempAlias([[^own_table_outer (\w+)$]], function()
                matches.writtenByTheScript = "still here"
                expandAlias("own_table_inner", false)
                seen.written = matches.writtenByTheScript
                seen.match = matches[2]
            end)
            finally(function()
                killAlias(innerId)
                killAlias(outerId)
            end)

            expandAlias("own_table_outer thing", false)

            assert.are.equal("still here", seen.written, "the caller was handed a rebuilt table rather than its own")
            assert.are.equal("thing", seen.match)
        end)

        -- What a command sent at the top level does to "command" is unchanged:
        -- every alias that command runs still sees it, including one reached
        -- after an earlier alias has nested a dispatch of its own
        it("still gives a command's own aliases the command that was sent", function()
            local seen = {}
            local firstId = tempAlias([[^sibling_probe (\w+)$]], function()
                seen.first = command
                expandAlias("sibling_nested", false)
            end)
            local nestedId = tempAlias([[^sibling_nested$]], function() end)
            local secondId = tempAlias([[^sibling_probe (\w+)$]], function()
                seen.second = command
                seen.secondMatch = matches[2]
            end)
            finally(function()
                killAlias(firstId)
                killAlias(nestedId)
                killAlias(secondId)
            end)

            expandAlias("sibling_probe thing", false)

            assert.are.equal("sibling_probe thing", seen.first)
            assert.are.equal("sibling_probe thing", seen.second, "a later alias for the same command saw the nested command instead")
            assert.are.equal("thing", seen.secondMatch)
        end)

        it("gives the caller its state back when the nested alias errors", function()
            local seen = {}
            local innerId = tempAlias([[^erroring_inner$]], function()
                error("a deliberate error from a spec's nested alias")
            end)
            local outerId = tempAlias([[^erroring_outer (\w+)$]], function()
                expandAlias("erroring_inner", false)
                seen.command = command
                seen.match = matches[2]
            end)
            finally(function()
                killAlias(innerId)
                killAlias(outerId)
            end)

            expandAlias("erroring_outer thing", false)

            assert.are.equal("erroring_outer thing", seen.command, "an erroring nested alias left the caller its command")
            assert.are.equal("thing", seen.match, "an erroring nested alias left the caller its captures")
        end)

        -- multimatches goes the same way as matches, which only a multiline
        -- trigger ever reads
        it("gives a multiline trigger back its multimatches", function()
            _G.NestedMultiSpec = {}
            local aliasId = tempAlias([[^nested_multi_alias$]], function() end)
            local code = [==[
                _G.NestedMultiSpec.before = multimatches[1][2] .. "," .. multimatches[2][2]
                expandAlias("nested_multi_alias", false)
                _G.NestedMultiSpec.after = multimatches[1][2] .. "," .. multimatches[2][2]
            ]==]
            tempComplexRegexTrigger("SpecNestedMulti", [[^nm one (\w+)$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 3)
            tempComplexRegexTrigger("SpecNestedMulti", [[^nm two (\w+)$]], code, 1, 0, 0, 0, 0, 0, 0, 0, 0, 3)
            finally(function()
                killTrigger("SpecNestedMulti")
                killAlias(aliasId)
                _G.NestedMultiSpec = nil
            end)

            feedTriggers("nm one aaa\n")
            feedTriggers("nm two bbb\n")

            assert.are.equal("aaa,bbb", _G.NestedMultiSpec.before, "the multiline trigger should see its own multimatches")
            assert.are.equal("aaa,bbb", _G.NestedMultiSpec.after, "multimatches was emptied by the nested expansion")
        end)

        -- The globals table carries a metatable - Mudlet puts a __call on it in
        -- Other.lua - and a package is free to add __index or __newindex to one
        -- of its own. Reading these globals through such a metatable can raise,
        -- and a raise inside expandAlias() skips the restore that pairs with the
        -- save, leaving the next restore to hand back some other caller's state.
        describe("with a metatable on the globals table", function()

            local savedMetatable, savedMultimatches, savedCommand

            before_each(function()
                savedMetatable = getmetatable(_G)
                savedMultimatches = rawget(_G, "multimatches")
                savedCommand = rawget(_G, "command")
            end)

            after_each(function()
                setmetatable(_G, savedMetatable)
                rawset(_G, "multimatches", savedMultimatches)
                rawset(_G, "command", savedCommand)
            end)

            it("reads the globals without running an __index", function()
                local seen = {}
                local innerId = tempAlias([[^meta_inner$]], function()
                    seen.innerRan = true
                end)
                local midId = tempAlias([[^meta_mid (\w+)$]], function()
                    -- absent, so a metatable is the only way the read can answer
                    rawset(_G, "multimatches", nil)
                    setmetatable(_G, {__index = function(_, key)
                        if key == "multimatches" then
                            error("a package's __index raised")
                        end
                        return nil
                    end})
                    seen.raised = not pcall(expandAlias, "meta_inner", false)
                    setmetatable(_G, savedMetatable)
                end)
                local outerId = tempAlias([[^meta_outer (\w+)$]], function()
                    expandAlias("meta_mid beta", false)
                    seen.outerMatch = matches[2]
                    seen.outerCommand = command
                end)
                finally(function()
                    killAlias(innerId)
                    killAlias(midId)
                    killAlias(outerId)
                end)

                expandAlias("meta_outer alpha", false)

                assert.is_false(seen.raised, "reading the globals ran a package's __index")
                assert.is_true(seen.innerRan, "the nested alias never ran")
                assert.are.equal("alpha", seen.outerMatch, "the outer alias was handed another caller's captures")
                assert.are.equal("meta_outer alpha", seen.outerCommand, "the outer alias was handed another caller's command")
            end)

            -- Handing the dispatch its own "command" is Mudlet's own write, and it
            -- goes in raw for the same reason the parking does. A raise from a
            -- package's __newindex there longjmps to the calling script's pcall
            -- from the middle of the dispatch, past the restore below and past
            -- every C++ destructor between - the command Host::send() split and
            -- expandAlias()'s own copy of it leak outright, which is the class
            -- CI/check-lua-error-strands.lua exists for.
            it("hands the dispatch its command without running a __newindex", function()
                local seen = {}
                local innerId = tempAlias([[^stranded_inner$]], function()
                    seen.innerRan = true
                    seen.innerCommand = command
                end)
                local midId = tempAlias([[^stranded_mid (\w+)$]], function()
                    -- absent, so a __newindex is the only thing the write can reach
                    rawset(_G, "command", nil)
                    setmetatable(_G, {__newindex = function(globals, key, value)
                        if key == "command" then
                            error("a package's __newindex raised")
                        end
                        rawset(globals, key, value)
                    end})
                    seen.raised = not pcall(expandAlias, "stranded_inner", false)
                    setmetatable(_G, savedMetatable)
                end)
                local outerId = tempAlias([[^stranded_outer (\w+)$]], function()
                    expandAlias("stranded_mid beta", false)
                    seen.outerMatch = matches[2]
                    seen.outerCommand = command
                end)
                finally(function()
                    killAlias(innerId)
                    killAlias(midId)
                    killAlias(outerId)
                end)

                expandAlias("stranded_outer alpha", false)

                assert.is_false(seen.raised, "setting the dispatch's command ran a package's __newindex")
                assert.is_true(seen.innerRan, "the nested alias never ran")
                assert.are.equal("stranded_inner", seen.innerCommand, "the nested alias was not given its own command")
                assert.are.equal("alpha", seen.outerMatch, "the outer alias was handed another caller's captures")
                assert.are.equal("stranded_outer alpha", seen.outerCommand, "the outer alias was handed another caller's command")
            end)

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

    describe("alias captures", function()

        after_each(function()
            _G.AliasSpec = nil
        end)

        it("puts a named group's capture in the matches table under its name", function()
            _G.AliasSpec = {}
            local id = tempAlias([[^named_alias (?<who>\w+) with (?<what>\w+)$]], [==[
                _G.AliasSpec.whole = matches[1]
                _G.AliasSpec.who = matches["who"]
                _G.AliasSpec.what = matches["what"]
            ]==])
            finally(function() killAlias(id) end)

            expandAlias("named_alias orc with sword", false)

            assert.are.equal("named_alias orc with sword", _G.AliasSpec.whole, "the alias should have matched at all")
            assert.are.equal("orc", _G.AliasSpec.who)
            assert.are.equal("sword", _G.AliasSpec.what)
        end)

        it("leaves out a named group that took no part in the match", function()
            _G.AliasSpec = {}
            local id = tempAlias([[^named_alias_alt (?:(?<left>aaa)|(?<right>bbb))$]], [==[
                _G.AliasSpec.left = matches["left"]
                _G.AliasSpec.right = matches["right"]
            ]==])
            finally(function() killAlias(id) end)

            expandAlias("named_alias_alt bbb", false)

            assert.are.equal("bbb", _G.AliasSpec.right, "the branch that matched should be named in the matches table")
            assert.is_nil(_G.AliasSpec.left, "a group on the branch that did not match has no capture to offer")
        end)

        it("gives an empty string for a group that matched no characters", function()
            _G.AliasSpec = {}
            local id = tempAlias([[^empty_capture_alias(\d*)$]], [==[
                _G.AliasSpec.whole = matches[1]
                _G.AliasSpec.digits = matches[2]
            ]==])
            finally(function() killAlias(id) end)

            expandAlias("empty_capture_alias", false)

            assert.are.equal("empty_capture_alias", _G.AliasSpec.whole, "the alias should have matched at all")
            assert.are.equal("", _G.AliasSpec.digits, "a group that matched nothing is still a capture")
        end)

        it("does not fire an alias whose pattern failed to compile", function()
            _G.AliasSpec = {good = 0, bad = 0}
            local goodId = tempAlias([[^bad_alias_pattern$]], [==[_G.AliasSpec.good = _G.AliasSpec.good + 1]==])
            local badId = tempAlias([[^bad_alias_pattern($]], [==[_G.AliasSpec.bad = _G.AliasSpec.bad + 1]==])
            -- busted keeps only the last finally(), so this undoes everything at once
            finally(function()
                killAlias(goodId)
                killAlias(badId)
            end)
            assert.is_true(badId > 0, "an uncompilable pattern still makes an alias, so that it can be seen and repaired")

            expandAlias("bad_alias_pattern", false)

            assert.are.equal(1, _G.AliasSpec.good, "the control alias shows the command does reach the alias engine")
            assert.are.equal(0, _G.AliasSpec.bad, "an alias whose regex did not compile must not fire")
        end)

        it("does not fire an alias with an empty pattern", function()
            _G.AliasSpec = {empty = 0, control = 0}
            local emptyId = tempAlias("", [==[_G.AliasSpec.empty = _G.AliasSpec.empty + 1]==])
            local controlId = tempAlias([[^empty_pattern_alias$]], [==[_G.AliasSpec.control = _G.AliasSpec.control + 1]==])
            finally(function()
                killAlias(emptyId)
                killAlias(controlId)
            end)
            assert.is_true(emptyId > 0, "an empty pattern still makes an alias, so that it can be seen and repaired")

            expandAlias("empty_pattern_alias", false)

            assert.are.equal(1, _G.AliasSpec.control, "the control alias shows the command does reach the alias engine")
            assert.are.equal(0, _G.AliasSpec.empty, "an empty pattern would otherwise match every command typed")
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

        it("freeing a temporary alias leaves a same-named permanent one reachable", function()
            -- tempAlias names its alias after its id, so a permanent alias called
            -- after that number shares the name - and the name lookup table holds
            -- several aliases per name
            local tempId = tempAlias("^spec_evicted_temp$", [[]])
            local sharedName = tostring(tempId)
            -- permanent aliases cannot be deleted from Lua, so earlier local runs
            -- can leave same-named ones behind: work from a relative baseline
            local before = exists(sharedName, "alias")
            assert.is_true(permAlias(sharedName, "", "^spec_evicted_perm$", [[]]) > 0)
            finally(function() disableAlias(sharedName) end)
            assert.are.equal(before + 1, exists(sharedName, "alias"))

            assert.is_true(killAlias(tempId), "the temporary alias is the one that can be killed")
            -- an incoming line runs every unit's deferred cleanup, which frees it
            feedTriggers("\nspec_alias_eviction_flush\n")

            assert.are.equal(before, exists(sharedName, "alias"), "only the temporary alias should leave the lookup table")
            assert.is_true(enableAlias(sharedName), "the permanent alias must still be reachable by name")
        end)

        it("killAlias finds a temporary alias behind a same-named permanent one", function()
            -- killAlias walks the root node list in creation order, so a permanent
            -- alias restored from the profile sits in front of this session's
            -- temporaries: it must be scanned past, not reported as a failure
            local seed = tempAlias("^spec_kill_order_seed$", [[]])
            killAlias(seed)
            -- permAlias itself takes seed + 1, so the next temporary takes seed + 2
            local sharedName = tostring(seed + 2)
            assert.is_true(permAlias(sharedName, "", "^spec_kill_order_perm$", [[]]) > 0)
            finally(function() disableAlias(sharedName) end)

            local tempId = tempAlias("^spec_kill_order_temp$", [[]])
            assert.are.equal(seed + 2, tempId, "ids should still be handed out in sequence")
            assert.is_true(killAlias(tempId), "killAlias must scan past the permanent alias")
        end)

    end)
end)
