-- TriggerUnit files every trigger below a root by the runs of text its patterns
-- need, and before a line is walked it stamps the triggers whose text the line
-- holds. TTrigger::match() then skips the children of a folder that were not
-- stamped - unless a trigger is mid-way through a multiline match, is being
-- held open, or is matched against a capture rather than the line. That is a
-- second reading of when a trigger can act, and when it is wrong a trigger
-- quietly stops firing, so these specs feed lines of every shape the index
-- files differently and require the same firings as without it.
--
-- The index is one-sided: it only ever says what cannot match, so a wrong
-- answer can only ever lose a firing. These specs therefore pin what must still
-- fire, and only run once the index is armed - which takes MUDLET_TEST_MODE to
-- report, and a few dozen filed triggers to engage, which the padding supplies.
-- MUDLET_TREE_INDEX=0 turns the index off and these report as pending.
--
-- Nothing in the Lua API builds a multiline, stay-open or filter trigger below
-- a folder, nor edits a pattern in place - hence the fixture package, and the
-- editor's pattern change is a manual check.
describe("trigger matching through the whole-tree index", function()

    local packageName = "mudlet-spec-treeindex"
    local folder = packageName .. " folder"
    local specDirectory = debug.getinfo(1, "S").source:match("^@(.*)[/\\]")
    assert(specDirectory, "TreeIndex_spec.lua has to be run from a file so that it can find its fixture")
    local fixture = specDirectory .. "/fixtures/packages/sources/" .. packageName .. "/" .. packageName .. ".xml"

    -- Enough filed triggers below one root to make the index engage; a
    -- rebuild reports how many it filed, which pins that they did
    local paddingTriggers = 40

    local fired

    local function note(key)
        fired[key] = (fired[key] or 0) + 1
    end

    local function feed(...)
        for _, line in ipairs({...}) do
            feedTriggers(line .. "\n")
        end
    end

    local function indexStats()
        local stats = getProfileStats().triggers
        return stats.treeIndexRebuilds, stats.treeIndexFiled
    end

    -- Without the index armed a pass here would say nothing about it
    local function itIndexed(name, body)
        it(name, function()
            local rebuilds, filed = indexStats()
            if not rebuilds then
                pending("telling whether the index is armed needs MUDLET_TEST_MODE")
            end
            if rebuilds == 0 then
                pending("the whole-tree index is off (MUDLET_TREE_INDEX=0)")
            end
            assert.is_true(filed >= paddingTriggers,
                           "the index filed " .. filed .. " triggers, fewer than the padding, so it has not engaged and this spec would prove nothing")
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

        -- Below the package's root folder, so that they are filed and the
        -- uninstall takes them away again: permanent triggers cannot be killed
        for index = 1, paddingTriggers do
            local id, message = permSubstringTrigger("treeindex padding " .. index, packageName,
                                                     {"treeindex_padding_" .. index .. "_bait"}, 'TreeIndexSpecNote("padding")')
            assert(id, message)
        end
        -- saved disabled, so that nothing in the fixture fires outside this spec
        enableTrigger(folder)
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
        local rebuildsBefore, filedBefore = indexStats()
        local id, message = permSubstringTrigger("treeindex late", packageName, {"treeindex_late_bait"}, 'TreeIndexSpecNote("late")')
        assert(id, message)

        feed("treeindex_late_bait")

        assert.are.same({late = 1}, fired)
        local rebuilds, filed = indexStats()
        assert.is_true(rebuilds > rebuildsBefore, "adding a trigger should have the index rebuilt on the next line")
        assert.are.equal(filedBefore + 1, filed, "the added trigger should be filed by that rebuild")
    end)

    itIndexed("fires a trigger a script creates on a line that script feeds in the same pass", function()
        local id, message = permSubstringTrigger("treeindex spawner", packageName, {"treeindex_spawner_bait"}, [[
            local spawnedId, spawnedMessage = permSubstringTrigger("treeindex spawned", "]] .. packageName .. [[",
                                                                   {"treeindex_spawned_bait"}, 'TreeIndexSpecNote("spawned")')
            assert(spawnedId, spawnedMessage)
            feedTriggers("treeindex_spawned_bait\n")
        ]])
        assert(id, message)

        feed("treeindex_spawner_bait")
        assert.are.same({spawned = 1}, fired, "a trigger created mid-pass is not filed yet, and must be walked rather than skipped")

        feed("treeindex_spawned_bait")
        assert.are.same({spawned = 2}, fired, "once filed, the same trigger must still fire")
    end)

    -- Last, as it takes the fixture away: a script's uninstall deletes filed
    -- triggers in the middle of a line, which the index's next filing has to
    -- survive - the sanitiser builds are what would catch it not doing so
    itIndexed("survives a script uninstalling the package its filed triggers are in", function()
        local rebuildsBefore = indexStats()
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
        local rebuilds = indexStats()
        assert.is_true(rebuilds > rebuildsBefore, "the deletions should have the index rebuilt on the next line")
    end)
end)
