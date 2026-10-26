-- TriggerUnit files every trigger below a root by the runs of text its patterns
-- need, stamps the ones a line holds before the tree is walked, and
-- TTrigger::match() skips the children of a folder that were not stamped. The
-- index is one-sided - it only ever says what cannot match - so a wrong answer
-- can only lose a firing, and these specs pin what must still fire. They run
-- once the index is armed, which takes MUDLET_TEST_MODE to report and a few
-- dozen filed triggers to engage; MUDLET_TREE_INDEX=0 makes them pending.
--
-- The fixture package holds the shapes the Lua API cannot build below a folder.
-- A pattern changed in the editor is a manual check.
describe("trigger matching through the whole-tree index", function()

    local packageName = "mudlet-spec-treeindex"
    local folder = packageName .. " folder"
    local specDirectory = debug.getinfo(1, "S").source:match("^@(.*)[/\\]")
    assert(specDirectory, "TreeIndex_spec.lua has to be run from a file so that it can find its fixture")
    local fixture = specDirectory .. "/fixtures/packages/sources/" .. packageName .. "/" .. packageName .. ".xml"

    -- the fixture's folder holds eleven triggers, of which the lua one cannot be filed
    local fixtureFiled = 10
    -- Enough filed triggers below one root to make the index engage
    local paddingTriggers = 40

    local fired
    local firedBeforePadding, filedBeforePadding

    local function note(key)
        fired[key] = (fired[key] or 0) + 1
    end

    local function feed(...)
        for _, line in ipairs({...}) do
            feedTriggers(line .. "\n")
        end
    end

    -- reported under test mode with the index on, as what the last rebuild filed
    local function filedCount()
        return getProfileStats().triggers.treeIndexFiled
    end

    -- Without the index armed a pass here would say nothing about it
    local function itIndexed(name, body)
        it(name, function()
            local filed = filedCount()
            if not filed then
                pending("telling whether the index is armed needs MUDLET_TEST_MODE and the index on")
            end
            assert.is_true(filed >= paddingTriggers + fixtureFiled,
                           "the index filed " .. filed .. " triggers, fewer than the padding and fixture, so it has not engaged and this spec would prove nothing")
            body()
        end)
    end

    -- the same retries as Trigger_spec.lua's fixtures, which says why
    local function packageInstalled()
        return table.contains(getPackages(), packageName)
    end

    local function waitForProfileSaveToPass()
        for _ = 1, 100 do
            if installPackage("") == nil then
                return
            end
            pumpEvents(50)
        end
    end

    local function removePackage()
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
        return not packageInstalled(), reason
    end

    setup(function()
        -- itIndexed() reports pending without test mode, so there is nothing
        -- to install for; installing also needs pumpEvents(), which does
        -- nothing without test mode
        if not os.getenv("MUDLET_TEST_MODE") then
            return
        end
        removePackage()
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

        -- saved disabled, so that nothing in the fixture fires outside this spec
        enableTrigger(folder)
        -- the fixture alone is too small for the index to engage
        fired = {}
        _G.TreeIndexSpecNote = note
        feed("there is treeindex_sub_bait on this line")
        _G.TreeIndexSpecNote = nil
        firedBeforePadding, filedBeforePadding = fired.substring, filedCount()

        -- Below the package's root folder, so that they are filed and the
        -- uninstall takes them away again: permanent triggers cannot be killed
        for index = 1, paddingTriggers do
            local id, message = permSubstringTrigger("treeindex padding " .. index, packageName,
                                                     {"treeindex_padding_" .. index .. "_bait"}, 'TreeIndexSpecNote("padding")')
            assert(id, message)
        end
        -- a changed tree is refiled on the next line fed from outside any trigger script
        feed("treeindex warm-up line")
    end)

    teardown(function()
        if packageInstalled() then
            disableTrigger(folder)
            local gone, reason = removePackage()
            assert.is_true(gone, "the " .. packageName .. " fixture was left behind: " .. tostring(reason))
        end
    end)

    before_each(function()
        fired = {}
        -- the fixture's and padding's scripts are source, so the counter has
        -- to be reachable by name from them
        _G.TreeIndexSpecNote = note
    end)

    after_each(function()
        _G.TreeIndexSpecNote = nil
    end)

    it("fires through a tree too small for the index to engage", function()
        if not filedCount() then
            pending("telling whether the index is armed needs MUDLET_TEST_MODE and the index on")
        end
        assert.are.equal(1, firedBeforePadding, "a trigger the index does not file must still fire")
        if filedBeforePadding ~= 0 then
            -- earlier specs leave permanent triggers behind, enough of them in a full run
            pending("other triggers in the profile had the index engaged before this spec's padding, so a tree too small for it cannot be made here")
        end
    end)

    itIndexed("fires each kind of pattern on its line and nothing on a line without bait", function()
        feed("treeindex quiet line that no trigger wants",
             "treeindex_padding_7_bai, almost",
             "there is treeindex_sub_bait on this line",
             "treeindex_prefix starts this line",
             "this line ends with treeindex_prefix",
             "treeindex_exact",
             "treeindex_exact and then some",
             "You gain 42 treeindex gold",
             "You gain some treeindex gold",
             "treeindex_lua",
             "not quite treeindex_lua",
             "treeindex_padding_7_bait")

        assert.are.same({substring = 1, prefix = 1, exact = 1, ["regex 42"] = 1, lua = 1, padding = 1}, fired)
    end)

    itIndexed("fires a trigger on whichever of its patterns the line holds", function()
        feed("treeindex_either_one", "treeindex_either_two", "treeindex_either_three")

        assert.are.same({either = 2}, fired)
    end)

    itIndexed("visits a regex trigger that needs no one run of text on every line", function()
        local filedBefore = filedCount()
        local id, message = permRegexTrigger("treeindex regex without literal", packageName,
                                             {[[^(?:\d{5}|treeindex_(?:one|two))$]]}, 'TreeIndexSpecNote("bareRegex")')
        assert(id, message)

        feed("12345", "treeindex_two", "1234")

        assert.are.same({bareRegex = 2}, fired)
        assert.are.equal(filedBefore, filedCount(), "a regex with no literal every match needs cannot be filed")
    end)

    itIndexed("completes a multiline trigger on a line that holds only its later pattern", function()
        feed("treeindex_ml_open", "the curtain falls")

        assert.are.same({multiline = 1}, fired, "a line holding only the second pattern must still reach a trigger that is mid-way")

        feed("quiet", "quiet", "quiet", "quiet", "the curtain falls")
        assert.are.same({multiline = 1}, fired, "nothing opened, so the closing pattern alone must not fire")

        feed("treeindex_ml_open", "quiet", "quiet", "quiet", "quiet", "the curtain falls")
        assert.are.same({multiline = 1}, fired, "the opening expired two lines on, so the close must not fire")
    end)

    itIndexed("passes lines without its own bait to the children of a trigger its match holds open", function()
        feed("treeindex_behind_gate")
        assert.are.same({}, fired, "the gate is shut, so nothing behind it may fire")

        feed("treeindex_gate_open", "treeindex_behind_gate", "treeindex_behind_gate", "treeindex_behind_gate")
        assert.are.same({gate = 1, gated = 2}, fired, "the gate stays open for two lines after its match, no more")

        feed("treeindex_gate_open and treeindex_behind_gate at once", "treeindex_behind_gate", "treeindex_behind_gate", "treeindex_behind_gate")
        assert.are.same({gate = 2, gated = 5}, fired, "a child with bait on the gate's own line fires on it, then twice more while it is held open")
    end)

    itIndexed("passes lines to the children of a trigger a script holds open", function()
        setTriggerStayOpen(packageName .. " gate", 2)

        feed("treeindex_behind_gate", "treeindex_behind_gate", "treeindex_behind_gate")
        assert.are.same({gated = 2}, fired)
    end)

    itIndexed("matches the child of a filter trigger against the capture", function()
        feed("treeindex filter divine", "treeindex filter mortal", "divine")

        assert.are.same({filterChild = 1}, fired)
    end)

    itIndexed("files a trigger added after the index was built", function()
        local filedBefore = filedCount()
        local id, message = permSubstringTrigger("treeindex late", packageName, {"treeindex_late_bait"}, 'TreeIndexSpecNote("late")')
        assert(id, message)

        feed("treeindex_late_bait")

        assert.are.same({late = 1}, fired)
        assert.are.equal(filedBefore + 1, filedCount(), "adding a trigger should have the index refiled on the next line, with it in")
    end)

    itIndexed("files a trigger added below a folder that was itself added", function()
        local filedBefore = filedCount()
        local groupId, groupMessage = permGroup("treeindex subfolder", "trigger", packageName)
        assert(groupId, groupMessage)
        local id, message = permSubstringTrigger("treeindex deep", "treeindex subfolder", {"treeindex_deep_bait"}, 'TreeIndexSpecNote("deep")')
        assert(id, message)

        feed("treeindex_deep_bait")

        assert.are.same({deep = 1}, fired)
        assert.are.equal(filedBefore + 1, filedCount(), "the trigger is filed and the folder, which has no patterns, is not")
    end)

    itIndexed("walks a trigger a script creates mid-line without refiling the tree before the line is done", function()
        local filedBefore = filedCount()
        local id, message = permSubstringTrigger("treeindex spawner", packageName, {"treeindex_spawner_bait"}, [[
            local spawnedId, spawnedMessage = permSubstringTrigger("treeindex spawned", "]] .. packageName .. [[",
                                                                   {"treeindex_spawned_bait"}, 'TreeIndexSpecNote("spawned")')
            assert(spawnedId, spawnedMessage)
            feedTriggers("a quiet line fed from the spawner's script\n")
        ]])
        assert(id, message)

        feed("treeindex_spawner_bait and treeindex_spawned_bait together")
        assert.are.same({spawned = 1}, fired,
                        "the line that created the trigger stamped before it existed, so filing it on the line its script fed would have it skipped")
        assert.are.equal(filedBefore + 1, filedCount(), "the line fed from the script must not refile the tree")

        feed("treeindex_spawned_bait")
        assert.are.same({spawned = 2}, fired, "once filed, the same trigger must still fire")
        assert.are.equal(filedBefore + 2, filedCount())
    end)

    itIndexed("enters a trigger a script holds open below a folder below a folder", function()
        assert(permGroup("treeindex open folder", "trigger", packageName))
        assert(permSubstringTrigger("treeindex open", "treeindex open folder", {"treeindex_open_bait"}, 'TreeIndexSpecNote("open")'))
        feed("treeindex warm-up line")

        setTriggerStayOpen("treeindex open", 2)
        feed("quiet", "quiet", "quiet")

        assert.are.same({open = 2}, fired, "held open for two lines, so it fires on both without its bait")
    end)

    itIndexed("enters a trigger below a folder on the line a script earlier in the walk opened it", function()
        -- made before the folder, so that the walk reaches the folder after the opener's script has run
        assert(permSubstringTrigger("treeindex opener", packageName, {"treeindex_opener_bait"}, [[
            TreeIndexSpecNote("opener")
            setTriggerStayOpen("treeindex held", 1)
        ]]))
        assert(permGroup("treeindex held folder", "trigger", packageName))
        assert(permSubstringTrigger("treeindex held", "treeindex held folder", {"treeindex_held_bait"}, 'TreeIndexSpecNote("held")'))
        feed("treeindex warm-up line")

        feed("treeindex_opener_bait")
        assert.are.same({opener = 1, held = 1}, fired, "opened for one line, the line the opener matched")

        feed("quiet")
        assert.are.same({opener = 1, held = 1}, fired, "the one line it was opened for is over")
    end)

    itIndexed("visits a trigger the index cannot file below a folder below a folder on every line", function()
        assert(permGroup("treeindex outer folder", "trigger", packageName))
        assert(permGroup("treeindex inner folder", "trigger", "treeindex outer folder"))
        assert(permRegexTrigger("treeindex nested regex", "treeindex inner folder", {[[^\d{6}$]]}, 'TreeIndexSpecNote("nestedRegex")'))
        feed("treeindex warm-up line")

        feed("654321", "quiet", "123456")

        assert.are.same({nestedRegex = 2}, fired, "nothing else on these lines is filed in either folder, so only the regex itself can have them entered")
    end)

    -- Last, as it takes the fixture away: a script's uninstall takes filed
    -- triggers away at the end of the line, which the index's next filing has
    -- to survive - the sanitiser builds are what would catch it not doing so
    itIndexed("survives a script uninstalling the package its filed triggers are in", function()
        local filedBefore = filedCount()
        waitForProfileSaveToPass()
        local removed, reason
        local id = tempTrigger("treeindex_uninstall_bait", function()
            removed, reason = uninstallPackage(packageName)
        end)
        finally(function()
            killTrigger(id)
        end)

        feed("treeindex_uninstall_bait")
        assert.is_true(removed == true, "could not uninstall the fixture from a trigger script: " .. tostring(reason))
        assert.is_false(packageInstalled())

        feed("treeindex_padding_7_bait", "treeindex_sub_bait")
        assert.are.same({}, fired, "the uninstalled triggers must be gone")
        assert.is_true(filedCount() <= filedBefore - paddingTriggers, "the deletions should have the index refiled on the next line, without them")
    end)
end)
