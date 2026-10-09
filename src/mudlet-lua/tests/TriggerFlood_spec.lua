-- A line arriving on its own and the same line arriving in the middle of a
-- burst do not take the same path through the trigger engine. A chunk carrying
-- MUDLET_MATCH_FLOOD_LINES lines or more (8 by default), in a profile whose
-- previous line ran MUDLET_MATCH_THRESHOLD regex searches or more (128) that
-- found no match, and MUDLET_MATCH_MISSES_PER_MATCH (2) of those for each one
-- that did, opens the parallel prescan in TriggerMatchPool: worker threads decide up front
-- which regex triggers cannot match the line, and TTrigger::match() then skips
-- those. The first line of a burst is judged by the line before the burst.
--
-- That decision is a second implementation of the matching rules, so if it ever
-- drifts from the first, triggers stop firing and nothing says so. These specs
-- feed the same lines both ways and require the same firings.
--
-- The pool is off unless switched on, in the settings or with
-- MUDLET_MATCH_THREADS=2 or more, as CI and .claude/scripts/run-lua-tests.sh do.
-- The counter that proves a burst reached the pool is reported under
-- MUDLET_TEST_MODE only. Without either there is nothing here worth running,
-- and these report as pending rather than passing on a comparison that never
-- happened.
--
-- Worth running under more than the defaults - see "Runtime tuning" in
-- docs/platform-builds.md. MUDLET_MATCH_SPIN_US=0 matters most here: the lines
-- these specs feed arrive far enough apart that a parked helper meets every
-- one of them cold, which is the path a warm burst never touches.
describe("trigger matching under a flood", function()

    -- Enough regex triggers, each searching every line, to clear the prescan's
    -- threshold without depending on what else the profile happens to have
    -- loaded. Only regex searches count: the other kinds are cheap enough to
    -- answer on the main thread, and so is a pattern with one run of text every
    -- match has to hold, which a line lacking it is dismissed from without
    -- pcre2 being asked. The alternation below is what leaves these with no
    -- such run, so that each of them really does search every line.
    local paddingTriggers = 130

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
                pending("the parallel prescan is off - switch it on with MUDLET_MATCH_THREADS=2")
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
            track(tempRegexTrigger("^(?:flood_padding_matches_nothing_" .. index .. "|flood_padding_never_" .. index .. ")$", function() note("padding") end))
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

    itFlood("fires a pattern whose required text the line holds only once encoded", function()
        -- A perl pattern that every match has to hold one run of literal text
        -- is dismissed from a line without that run before pcre2 is asked, on
        -- the prescan's threads as much as on this one. The two look for the
        -- run in the line as Qt holds it but run the pattern against its UTF-8,
        -- and those carry the same text for every line but one: an unpaired
        -- surrogate has no UTF-8 of its own, so encoding drops it and puts the
        -- text either side of it together. "abcd" is not in this line - the
        -- surrogate splits it - and is in the bytes pcre2 reads, so a prescan
        -- that skips the dismissal for such a line keeps the trigger and one
        -- that does not loses a match nothing would report.
        track(tempRegexTrigger("abcd", function() note("split") end))
        setConfig("specialForceMXPProcessorOn", true)
        finally(function() setConfig("specialForceMXPProcessorOn", false) end)

        feedAsBurst(filler(12, {[6] = "zqab&#xD800;cd"}))

        assert.are.equal(1, fired.split,
                         "a burst dropped a match whose required text only the encoded line holds")
    end)

    itFlood("fires a pattern whose required text the line holds outright", function()
        -- The other side of the same dismissal, on a line whose UTF-8 holds
        -- what it holds: "flood_literal_" is the run every match needs, so the
        -- line carrying it reaches pcre2 and fires, the line spelling it with
        -- spaces is dismissed before pcre2 is asked, and the line carrying the
        -- run but not matching is dismissed by pcre2 itself. All three have to
        -- come out of a burst the way they come out of a trickle.
        track(tempRegexTrigger([[^flood_literal_(\d+)_tail$]], function() note("literal") end))

        feedAsBurst(filler(12, {
            [4] = "flood_literal_7_tail",
            [8] = "flood literal 7 tail",
            [10] = "flood_literal_seven_tail",
        }))

        assert.are.equal(1, fired.literal,
                         "a burst should keep a pattern whose required text the line holds, and only that line")
    end)

    itFlood("fires a pattern that opts out of JIT compilation", function()
        -- pcre2_jit_compile() accepts a (*NO_JIT) pattern without making any JIT
        -- code, so the prescan's threads must match it the way the main thread does
        track(tempRegexTrigger([[(*NO_JIT)^flood_nojit_(\w+)$]], function() note("noJit") end))

        feedAsBurst(filler(12, {[6] = "flood_nojit_line"}))

        assert.are.equal(1, fired.noJit, "a burst dropped a match of a (*NO_JIT) pattern")
    end)

    -- The burst's first line is judged by whatever line came before it, so a
    -- burst the pool stays out of may still reach it once
    local function prescansDuring(lines)
        local before = getProfileStats().triggers.prescans
        feedTriggers(table.concat(lines, "\n") .. "\n")
        return getProfileStats().triggers.prescans - before
    end

    local function hitLines()
        local lines = {}
        for index = 1, 12 do
            lines[index] = "flood_hit " .. index
        end
        return lines
    end

    itFlood("leaves a burst to the main thread when most of its regex searches match", function()
        -- A search that matches is reused, but still costs more with the pool
        -- than without, so a burst like this must not open it. The padding adds
        -- 130 searches that fail on every line; these add twice that many that
        -- match. Counting every search, as MUDLET_MATCH_MISSES_PER_MATCH=0 asks,
        -- opens it on every line.
        for index = 1, 2 * paddingTriggers do
            track(tempRegexTrigger("^(?:flood_hit|flood_also_hit_" .. index .. ") (\\d+)", function() note("hit") end))
        end

        local sampledBefore = getProfileStats().triggers.matchPool.sampledLines
        local prescans = prescansDuring(hitLines())
        -- A line the statistics sample is one the pool would have taken
        local sampled = getProfileStats().triggers.matchPool.sampledLines - sampledBefore

        assert.are.equal(12 * 2 * paddingTriggers, fired.hit, "every trigger should fire on every line")
        if os.getenv("MUDLET_MATCH_MISSES_PER_MATCH") == "0" then
            assert.is_true(prescans + sampled >= 11, "counting every search should open the pool, but it opened " .. prescans .. " times")
        elseif os.getenv("MUDLET_MATCH_MISSES_PER_MATCH") then
            pending("MUDLET_MATCH_MISSES_PER_MATCH changes how many matches keep a burst off the pool")
        else
            assert.is_true(prescans <= 1, "the burst went through the parallel prescan " .. prescans .. " times although most of its searches matched")
        end
    end)

    itFlood("leaves a burst to the main thread when too few of its regex searches fail", function()
        -- 140 searches a line, over the threshold, but only the 100 that fail are
        -- work the pool could take away, and those are under it
        if os.getenv("MUDLET_MATCH_THRESHOLD") or os.getenv("MUDLET_MATCH_MISSES_PER_MATCH") then
            pending("this measures the default threshold and misses per match")
        end
        for _, id in ipairs(ids) do
            disableTrigger(id)
        end
        for index = 1, 100 do
            track(tempRegexTrigger("^(?:flood_hit|flood_also_hit_" .. index .. ") (\\d+) never$", function() note("miss") end))
        end
        for index = 1, 40 do
            track(tempRegexTrigger("^(?:flood_hit|flood_more_hit_" .. index .. ") (\\d+)", function() note("hit") end))
        end

        local prescans = prescansDuring(hitLines())

        assert.are.equal(12 * 40, fired.hit, "every matching trigger should fire on every line")
        assert.is_nil(fired.miss)
        assert.is_true(prescans <= 1, "the burst went through the parallel prescan " .. prescans .. " times on 100 failed searches a line")
    end)

    -- A regex the prescan finds a match for is not searched for again: match()
    -- takes the captures the prescan's thread left in the trigger's match data.
    -- So the specs below hold a burst to the trickle's captures and positions,
    -- and check that the burst really did take some, or they compare nothing.
    describe("captures a burst takes from the prescan", function()

        local log

        local function record(label)
            local selected, start, length
            if selectCaptureGroup(2) ~= -1 then
                selected, start, length = getSelection()
            end
            local parts = {label}
            for index, value in ipairs(matches) do
                parts[#parts + 1] = index .. "=" .. value
            end
            for _, name in ipairs({"who", "amount"}) do
                if matches[name] then
                    parts[#parts + 1] = name .. "=" .. matches[name]
                end
            end
            parts[#parts + 1] = "at " .. tostring(selected) .. "@" .. tostring(start) .. "+" .. tostring(length)
            log[#log + 1] = table.concat(parts, " ")
        end

        -- Feeds the lines one at a time and then as a burst, and returns both logs
        local function trickleThenBurst(lines)
            log = {}
            for _, line in ipairs(lines) do
                feedTriggers(line .. "\n")
            end
            local trickle = log
            log = {}
            local reusedBefore = getProfileStats().triggers.prescanMatchesReused
            assert.is_not_nil(reusedBefore, "this build does not count captures taken from the prescan")
            feedAsBurst(lines)
            assert.is_true(getProfileStats().triggers.prescanMatchesReused > reusedBefore,
                           "no capture was taken from the prescan, so the burst compared nothing")
            return trickle, log
        end

        after_each(function()
            _G.FloodCaptureRecord = nil
        end)

        itFlood("hands a regex trigger the captures and positions it gets from a trickle", function()
            _G.FloodCaptureRecord = record
            track(tempRegexTrigger([[^flood_capture (\w+) (\d+)]], function() record("plain") end))
            track(tempRegexTrigger([[^flood_named (?<who>\w+) gives (?<amount>\d+)$]], function() record("named") end))
            track(tempRegexTrigger([[^flood_wide (\S+) (\S+) end$]], function() record("wide") end))
            track(tempRegexTrigger([[(*NO_JIT)^flood_nojit_capture (\w+) (\w+)$]], function() record("noJit") end))
            -- More groups than a prescan thread's scratch has room for
            track(tempRegexTrigger("^flood_many" .. string.rep(" (\\w)", 40) .. "$", function() record("many") end))
            -- Two patterns of one trigger, only the second of which matches
            track(tempComplexRegexTrigger("FloodTwoPatterns", [[^flood_never_this (\w+)$]], [==[FloodCaptureRecord("second")]==],
                                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
            -- which makes the trigger again, under a new id, with both patterns
            track(tempComplexRegexTrigger("FloodTwoPatterns", [[^flood_second (\w+) (\w+)$]], [==[FloodCaptureRecord("second")]==],
                                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0))

            local trickle, burst = trickleThenBurst(filler(12, {
                [2] = "flood_capture alpha 12 and the rest",
                [4] = "flood_named Cass gives 300",
                [5] = "flood_wide naïve→ ✓ü𝄞 end",
                [7] = "flood_nojit_capture left right",
                [9] = "flood_second one two",
                [11] = "flood_capture beta 7",
                [12] = "flood_many" .. string.rep(" q", 39) .. " z",
            }))

            assert.are.equal(7, #trickle, "every trigger should fire once on its line when fed one at a time")
            assert.are.same(trickle, burst)
        end)

        itFlood("hands a match-all trigger every match it gets from a trickle", function()
            -- every match and its captures, one after another, in matches
            _G.FloodCaptureRecord = record
            track(tempComplexRegexTrigger("FloodMatchAll", [[flood_g(\d)(\w?)]], [==[FloodCaptureRecord("all")]==],
                                          0, 0, 0, 0, 1, 0, 0, 0, 0, 0))

            local trickle, burst = trickleThenBurst(filler(12, {
                [3] = "flood_g1 flood_g2x then flood_g3",
                [8] = "één flood_g4é flood_g5",
            }))

            assert.are.equal(2, #trickle, "the match-all trigger should fire once per line when fed one at a time")
            assert.are.same(trickle, burst)
        end)

        itFlood("does not hand a trigger the captures of a line an earlier trigger fed in the meantime", function()
            -- The earlier trigger feeds a line the later one also matches, so the
            -- later one searches that line, into the same match data the prescan
            -- left the outer line's match in, before the outer line reaches it.
            track(tempRegexTrigger([[^flood_stale outer]], function() feedTriggers("flood_stale nested 22\n") end))
            track(tempRegexTrigger([[^flood_stale (\w+) (\d+)$]], function() record("later") end))

            local trickle, burst = trickleThenBurst(filler(12, {[6] = "flood_stale outer 1", [9] = "flood_stale other 3"}))

            assert.are.equal(3, #trickle, "the later trigger should fire on the fed line and both of its own")
            assert.truthy(trickle[1]:find("2=nested 3=22", 1, true), trickle[1])
            assert.truthy(trickle[2]:find("2=outer 3=1", 1, true), trickle[2])
            assert.truthy(trickle[3]:find("2=other 3=3", 1, true), trickle[3])
            assert.are.same(trickle, burst)
        end)

        itFlood("does not hand a trigger the captures of a burst an earlier trigger fed in the meantime", function()
            -- As above, but the fed lines are a burst of their own, which runs a
            -- prescan of its own while the outer line's is still to be used
            local nested = filler(10, {[5] = "flood_stale nested 22"})
            local nestedPrescans = 0
            track(tempRegexTrigger([[^flood_stale outer]], function()
                local before = getProfileStats().triggers.prescans
                feedTriggers(table.concat(nested, "\n") .. "\n")
                nestedPrescans = nestedPrescans + getProfileStats().triggers.prescans - before
            end))
            track(tempRegexTrigger([[^flood_stale (\w+) (\d+)$]], function() record("later") end))

            local trickle, burst = trickleThenBurst(filler(12, {[6] = "flood_stale outer 1", [9] = "flood_stale other 3"}))

            assert.is_true(nestedPrescans > 0, "the fed lines never reached the prescan, so this compared nothing")
            assert.are.equal(3, #trickle)
            assert.are.same(trickle, burst)
        end)

        itFlood("captures the line it was given after an earlier trigger rewrote it", function()
            -- Triggers go on matching the line as it arrived, whatever an earlier
            -- script has since done to the buffer, so a match the prescan found
            -- before the rewrite is still the one to report
            track(tempRegexTrigger([[^flood_rewrite (\w+)]], function()
                selectString(matches[2], 1)
                replace("REPLACED")
                insertText(" inserted ")
            end))
            track(tempRegexTrigger([[^flood_delete]], function() deleteLine() end))
            track(tempRegexTrigger([[^flood_(rewrite|delete) (\w+) (\d+)$]], function() record("later") end))

            local trickle, burst = trickleThenBurst(filler(12, {[4] = "flood_rewrite word 5", [8] = "flood_delete gone 6"}))

            assert.are.equal(2, #trickle)
            assert.are.same(trickle, burst)
        end)

        itFlood("captures with triggers an earlier trigger killed or created on the same line", function()
            -- Each swap line kills the trigger the prescan judged and makes another
            -- in its place, which the prescan never saw and so searches for itself
            local current
            local function makeSwapped()
                current = track(tempRegexTrigger([[^flood_swap (\w+) (\d+)$]], function() record("swapped") end))
            end
            track(tempRegexTrigger([[^flood_swap]], function()
                killTrigger(current)
                makeSwapped()
            end))
            makeSwapped()

            log = {}
            local lines = filler(12, {[5] = "flood_swap first 1", [10] = "flood_swap second 2"})
            for _, line in ipairs(lines) do
                feedTriggers(line .. "\n")
            end
            local trickle = log
            log = {}
            feedAsBurst(lines)

            assert.are.equal(2, #trickle, "only the trigger made on each swap line should fire on it")
            assert.truthy(trickle[1]:find("2=first 3=1", 1, true), trickle[1])
            assert.truthy(trickle[2]:find("2=second 3=2", 1, true), trickle[2])
            assert.are.same(trickle, log)
        end)
    end)

    it("reports on the pool in the profile statistics whether it is on or off", function()
        local report = getProfileStats().triggers.matchPool
        assert.is_table(report)
        assert.is_number(report.threads)
        assert.is_number(report.pooledLines)
        assert.is_number(report.sampledLines)
        assert.is_number(report.savedMilliseconds)
        assert.truthy(({["not enough data"] = true, ["worth it"] = true, ["about even"] = true, ["not worth it"] = true})[report.verdict],
                      tostring(report.verdict))
    end)

    itFlood("reports lines matched with the pool and a sample matched without it", function()
        -- Fires on every line, so a sampled one has to fire as well
        track(tempRegexTrigger([[^flood quiet filler \d+$]], function() note("filler") end))
        local before = getProfileStats().triggers.matchPool
        -- Comfortably more than one sampling period, 32 by default, of lines the pool would take
        feedAsBurst(filler(100))

        local after = getProfileStats().triggers.matchPool
        assert.are.equal(100, fired.filler, "every line should fire its trigger, sampled or not")
        assert.is_true(after.pooledLines > before.pooledLines, "no line of the burst was counted as matched with the pool")
        assert.is_true(after.pooledMicroseconds > 0)
        assert.is_true(after.threads >= 2)
        if os.getenv("MUDLET_MATCH_SAMPLE_EVERY") == "0" then
            assert.are.equal(before.sampledLines, after.sampledLines, "a line was sampled although sampling is off")
        elseif os.getenv("MUDLET_MATCH_SAMPLE_EVERY") == nil then
            assert.is_true(after.sampledLines - before.sampledLines >= 2,
                           "a burst of 100 lines should sample about one in 32, but sampled " .. (after.sampledLines - before.sampledLines))
            assert.is_true(after.sampledMicroseconds > 0)
        end
    end)

    -- Only a trigger with a regex among its patterns is prescanned, so the
    -- prescan judges the other kinds only when they share a trigger with one.
    -- No Lua API mixes pattern kinds in one trigger, hence the fixture.
    describe("triggers that mix a regex with other pattern kinds", function()

        local packageName = "mudlet-spec-floodmixed"
        local specDirectory = debug.getinfo(1, "S").source:match("^@(.*)[/\\]")
        assert(specDirectory, "TriggerFlood_spec.lua has to be run from a file so that it can find its fixtures")
        local fixture = specDirectory .. "/fixtures/packages/sources/" .. packageName .. "/" .. packageName .. ".xml"

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
            -- itFlood() reports pending without test mode or a working pool,
            -- so there is nothing to install for; installing also needs
            -- pumpEvents(), which does nothing without test mode
            local workers = getProfileStats().triggers.prescanWorkers
            if not os.getenv("MUDLET_TEST_MODE") or not workers or workers < 2 then
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
        end)

        teardown(function()
            if packageInstalled() then
                local gone, reason = removePackage()
                assert.is_true(gone, "the " .. packageName .. " fixture was left behind: " .. tostring(reason))
            end
        end)

        itFlood("fires the same way whether the lines trickle in or arrive at once", function()
            -- saved disabled, so that nothing in the fixture fires outside this spec
            enableTrigger(packageName .. " text kinds")
            enableTrigger(packageName .. " lua kind")
            finally(function()
                disableTrigger(packageName .. " text kinds")
                disableTrigger(packageName .. " lua kind")
            end)

            -- every kind gets a line it matches and a near miss next to it
            local corpus = filler(12, {
                [2] = "there is flood mixed bait on this line",
                [3] = "bait mixed flood, the words but not the phrase",
                [4] = "flood_mixed_prefix starts this line",
                [5] = "this line ends with flood_mixed_prefix",
                [6] = "flood_mixed_exact",
                [7] = "flood_mixed_exact and then some",
                [9] = "flood_mixed_lua",
                [10] = "not quite flood_mixed_lua",
            })
            local kinds = {"flood mixed bait", "flood_mixed_prefix", "flood_mixed_exact", "lua"}

            for _, line in ipairs(corpus) do
                feedTriggers(line .. "\n")
            end
            for _, key in ipairs(kinds) do
                assert.are.equal(1, fired[key], key .. " should fire once when the lines are fed one at a time")
            end

            feedAsBurst(corpus)

            -- every kind that went astray is named at once, rather than just the first
            local astray = {}
            for _, key in ipairs(kinds) do
                if fired[key] ~= 2 then
                    astray[#astray + 1] = key .. " fired " .. tostring(fired[key]) .. " times over both runs"
                end
            end
            assert.are.same({}, astray, "each kind fired once when the same lines arrived one at a time")
        end)
    end)
end)
