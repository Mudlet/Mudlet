-- What the matches table holds for a capture group that took no part in the
-- match. Scripts test `if matches[N] then` on optional trailing groups, so an
-- unset trailing group has to stay nil rather than become "". Triggers and
-- aliases build matches with separate copies of the same capture code, so
-- both are pinned here.
describe("matches for capture groups that did not participate", function()

    local function copyOf(source)
        local copy = {}
        for key, value in pairs(source) do
            copy[key] = value
        end
        return copy, #source
    end

    local function viaTrigger(pattern, line)
        local seen, length
        local id = tempRegexTrigger(pattern, function()
            seen, length = copyOf(matches)
        end)
        finally(function()
            killTrigger(id)
        end)
        feedTriggers("\n" .. line .. "\n")
        assert.is_table(seen, "the trigger should have matched at all")
        return seen, length
    end

    local function viaAlias(pattern, command)
        local seen, length
        local id = tempAlias(pattern, function()
            seen, length = copyOf(matches)
        end)
        finally(function()
            killAlias(id)
        end)
        expandAlias(command, false)
        assert.is_table(seen, "the alias should have matched at all")
        return seen, length
    end

    for _, case in ipairs({ { "in a trigger", viaTrigger }, { "in an alias", viaAlias } }) do
        local where, capture = case[1], case[2]

        describe(where, function()

            it("gives an empty string for an unset group before a set one", function()
                local seen, length = capture([[^unsetMiddle (a)(z)?(b)$]], "unsetMiddle ab")

                assert.are.equal("unsetMiddle ab", seen[1])
                assert.are.equal("a", seen[2])
                assert.are.equal("", seen[3])
                assert.are.equal("b", seen[4])
                assert.are.equal(4, length)
            end)

            it("leaves out unset groups after the last set one", function()
                local seen, length = capture([[^unsetTrailing (a)(b)(z)?(y)?$]], "unsetTrailing ab")

                assert.are.equal("unsetTrailing ab", seen[1])
                assert.are.equal("a", seen[2])
                assert.are.equal("b", seen[3])
                assert.is_nil(seen[4])
                assert.is_nil(seen[5])
                assert.are.equal(3, length)
            end)

            it("gives nil for an unset named group, wherever it is", function()
                local seen, length = capture([[^unsetNamed (?<head>a)(?<middle>z)?(?<tail>b)(?<last>y)?$]], "unsetNamed ab")

                assert.are.equal("a", seen.head)
                assert.is_nil(seen.middle)
                assert.are.equal("b", seen.tail)
                assert.is_nil(seen.last)
                -- the numbered entries follow the rules above
                assert.are.equal("", seen[3])
                assert.is_nil(seen[5])
                assert.are.equal(4, length)
            end)
        end)
    end

    local function viaMatchAllTrigger(pattern, line)
        local seen, length
        -- killed by name: tempComplexRegexTrigger names the trigger after its first argument, not its id
        local name = "matchesUnsetGroupsMatchAll"
        tempComplexRegexTrigger(name, pattern, function()
            seen, length = copyOf(matches)
        end, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0)
        finally(function()
            killTrigger(name)
        end)
        feedTriggers("\n" .. line .. "\n")
        assert.is_table(seen, "the trigger should have matched at all")
        return seen, length
    end

    for _, case in ipairs({ { "in a match-all trigger", viaMatchAllTrigger }, { "in an alias", viaAlias } }) do
        local where, capture = case[1], case[2]

        it(where .. ", lets a later match fill the index an unset trailing group left out", function()
            local seen, length = capture([[(q)(z)?]], "qq")

            assert.are.equal("q", seen[3])
            assert.are.same({ "q", "q", "q", "q" }, seen)
            assert.are.equal(4, length)
        end)
    end
end)
