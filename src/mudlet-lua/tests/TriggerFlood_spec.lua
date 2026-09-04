-- A line arriving on its own and the same line arriving in the middle of a
-- burst do not take the same path through the trigger engine. A chunk carrying
-- MUDLET_MATCH_FLOOD_LINES lines or more (8 by default), in a profile with
-- MUDLET_MATCH_THRESHOLD pattern-bearing triggers or more (24), opens the
-- parallel prescan in TriggerMatchPool: worker threads decide up front which
-- triggers cannot match the line, and TTrigger::match() then skips those.
--
-- That decision is a second implementation of the matching rules, so if it ever
-- drifts from the first, triggers stop firing and nothing says so. These specs
-- feed the same lines both ways and require the same firings.
--
-- The pool needs two threads to share a batch between (MUDLET_MATCH_THREADS,
-- default min(4, cores / 2)) and turns itself off below that, so by default a
-- machine with fewer than four cores never runs it. The counter that proves a
-- burst reached the pool is reported under MUDLET_TEST_MODE only. Without
-- either there is nothing here worth running, and these report as pending
-- rather than passing on a comparison that never happened.
describe("trigger matching under a flood", function()

    -- Enough pattern-bearing triggers to clear the prescan's threshold without
    -- depending on what else the profile happens to have loaded.
    local paddingTriggers = 40

    local ids
    local fired

    local function track(id)
        ids[#ids + 1] = id
        return id
    end

    local function note(key)
        fired[key] = (fired[key] or 0) + 1
    end

    local function filler(count, replacements)
        local lines = {}
        for index = 1, count do
            lines[index] = "flood quiet filler " .. index
        end
        for index, text in pairs(replacements or {}) do
            lines[index] = text
        end
        return lines
    end

    -- Nothing here is worth running without the pool, and a pass would be a lie
    -- about a comparison that never happened.
    local function itFlood(name, body)
        it(name, function()
            local workers = getProfileStats().triggers.prescanWorkers
            if not workers then
                pending("counting what reaches the prescan needs MUDLET_TEST_MODE")
            end
            -- the count includes the calling thread, and is zero when the
            -- pool declined to start, so this asks whether the parallel path
            -- exists at all rather than how wide it is
            if workers < 2 then
                pending("this machine has too few cores to run the parallel prescan")
            end
            body()
        end)
    end

    -- Every burst goes through here so that no spec below can quietly pass on a
    -- run where the prescan never engaged, which is what a broken flood gate
    -- would otherwise look like.
    local function feedAsBurst(lines)
        local before = getProfileStats().triggers.prescans
        feedTriggers(table.concat(lines, "\n") .. "\n")
        assert.is_true(getProfileStats().triggers.prescans > before,
                       "the burst did not reach the parallel prescan, so this spec compared nothing")
    end

    before_each(function()
        ids = {}
        fired = {}
        -- permRegexTrigger takes its script as source, so the counter has to be
        -- reachable by name from it.
        _G.FloodSpecNote = note
        for index = 1, paddingTriggers do
            track(tempTrigger("flood_padding_matches_nothing_" .. index, function() note("padding") end))
        end
    end)

    after_each(function()
        -- Newest first, so a child goes before the parent that owns it.
        for index = #ids, 1, -1 do
            disableTrigger(ids[index])
            killTrigger(ids[index])
        end
        ids = nil
        _G.FloodSpecNote = nil
    end)

    itFlood("fires the same triggers whether the lines trickle in or arrive at once", function()
        track(tempTrigger("flood substring bait", function() note("substring") end))
        track(tempBeginOfLineTrigger("flood_prefix", function() note("beginOfLine") end))
        track(tempExactMatchTrigger("flood_exact_line", function() note("exact") end))
        track(tempRegexTrigger([[^You gain (\d+) gold]], function() note("regex") end))

        local corpus = filler(12, {
            [2] = "there is flood substring bait on this line",
            [4] = "flood_prefix and then some",
            [6] = "flood_exact_line",
            [8] = "You gain 15 gold from the corpse.",
            [10] = "flood_prefix again",
            [12] = "and flood substring bait once more",
        })

        -- One line per call is below the flood threshold, so no prescan runs and
        -- this is the behaviour the burst below has to reproduce.
        for _, line in ipairs(corpus) do
            feedTriggers(line .. "\n")
        end
        local trickle = {}
        for key, count in pairs(fired) do
            trickle[key] = count
        end

        assert.are.equal(2, trickle.substring, "substring trigger, lines fed one at a time")
        assert.are.equal(2, trickle.beginOfLine, "begin-of-line trigger, lines fed one at a time")
        assert.are.equal(1, trickle.exact, "exact-match trigger, lines fed one at a time")
        assert.are.equal(1, trickle.regex, "regex trigger, lines fed one at a time")
        assert.is_nil(trickle.padding, "a padding trigger matched the corpus, so the comparison below proves nothing")

        feedAsBurst(corpus)

        for _, key in ipairs({"substring", "beginOfLine", "exact", "regex"}) do
            assert.are.equal(2 * trickle[key], fired[key],
                             key .. " fired " .. tostring(fired[key]) .. " times over both runs, but "
                             .. tostring(trickle[key]) .. " when the same lines arrived one at a time")
        end
    end)

    itFlood("fires a filter chain's child on a capture the line itself does not match", function()
        -- The child is anchored, so it matches the capture and never the whole
        -- line: judging it against the line would rule it out wrongly.
        track(tempComplexRegexTrigger("FloodFilterParent", [[^You gain (\w+) essence\.$]], [==[ ]==],
                                      0, 0, 0, 1, 0, 0, 0, 0, 0, 0))
        track(permRegexTrigger("FloodFilterChild", "FloodFilterParent", {[[^divine$]]},
                               [==[FloodSpecNote("filterChild")]==]))

        feedAsBurst(filler(11, {[6] = "You gain divine essence."}))

        assert.are.equal(1, fired.filterChild, "a filter chain's child should still see the parent's capture in a burst")
    end)

    itFlood("fires triggers whose outcome the line text alone does not decide", function()
        -- Multiline state, a line counter and a colour scan all depend on more
        -- than the text of the line, so the prescan has to let all three through.
        local multilineCode = [==[FloodSpecNote("multiline")]==]
        track(tempComplexRegexTrigger("FloodMultiline", [[^flood_multiline_one$]], multilineCode,
                                      1, 0, 0, 0, 0, 0, 0, 0, 0, 4))
        tempComplexRegexTrigger("FloodMultiline", [[^flood_multiline_two$]], multilineCode,
                                1, 0, 0, 0, 0, 0, 0, 0, 0, 4)
        -- Green on red, which no ordinary line carries.
        track(tempAnsiColorTrigger(2, 1, [==[FloodSpecNote("colour")]==]))
        local lineTrigger = track(tempLineTrigger(0, 20, [==[FloodSpecNote("line")]==]))

        local corpus = filler(12, {
            [3] = "flood_multiline_one",
            [5] = "flood_multiline_two",
            [9] = "\27[32;41mflood coloured line\27[0m",
        })
        feedAsBurst(corpus)
        -- A line trigger fires on position rather than text, so it has to stop
        -- before any later spec feeds a line.
        disableTrigger(lineTrigger)

        assert.are.equal(1, fired.multiline, "a multiline trigger should complete inside a burst")
        assert.are.equal(1, fired.colour, "a colour trigger should fire inside a burst")
        assert.is_true((fired.line or 0) >= #corpus,
                       "a line trigger should fire for every line of the burst, fired " .. tostring(fired.line or 0))
    end)

    itFlood("fires a trigger that an earlier trigger in the same burst enabled", function()
        -- The prescan judged this one while it was still inactive, and an
        -- inactive trigger has no verdict worth keeping: by the time the line
        -- reaches it, it is live and its pattern matches.
        local lateId = track(tempRegexTrigger([[^flood_late_line$]], function() note("late") end))
        disableTrigger(lateId)
        track(tempRegexTrigger([[^flood_enable_the_late_one$]], function() enableTrigger(lateId) end))

        feedAsBurst(filler(12, {
            [4] = "flood_enable_the_late_one",
            [9] = "flood_late_line",
        }))

        assert.are.equal(1, fired.late, "a trigger enabled mid-burst should fire on a later line of the same burst")
    end)

    itFlood("keeps a stay-open trigger's children running over the lines that follow", function()
        -- mKeepFiring is what carries a stay-open trigger past the line it
        -- matched, and it can be raised after the prescan has already run, so
        -- the skip has to re-read it rather than trust the verdict.
        track(tempComplexRegexTrigger("FloodStayOpen", [[^flood_open_the_window$]], [==[ ]==],
                                      0, 0, 0, 0, 0, 0, 0, 0, 6, 0))
        track(permRegexTrigger("FloodStayOpenChild", "FloodStayOpen", {[[flood quiet filler]]},
                               [==[FloodSpecNote("stayOpenChild")]==]))

        feedAsBurst(filler(12, {[3] = "flood_open_the_window"}))

        assert.is_true((fired.stayOpenChild or 0) > 0,
                       "a stay-open trigger should keep offering later lines to its children during a burst")
    end)
end)
