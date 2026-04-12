describe("PCRE regex cases with tempRegexTrigger", function()

    before_each(function()
    _G.matches = nil
    end)

    -- start/end anchors (^ and $)
    it("matches only when the entire line fits between ^ and $", function()
        local send = spy.on(_G, "send")
        local snapshot = {}
        local pattern = "^(\\w+)\\s+(\\w+)$"

        local id = tempRegexTrigger(pattern, function()
            send("match")
            snapshot = matches
        end, 1)

        feedTriggers("\nHello Mudlet\n")

        assert.spy(send).was.called(1)
        assert.is_table(snapshot)
        assert.are.equal("Hello Mudlet", snapshot[1])
        assert.are.equal("Hello", snapshot[2])
        assert.are.equal("Mudlet", snapshot[3])
        assert.is_nil(snapshot[4])
        killTrigger(id)
    end)

    -- character classes and fixed width (\d, \w, \s, {m})
    it("extracts digits and words using classes", function()
        local send = spy.on(_G, "send")
        local snapshot = {}
        local pattern = "^(\\d{4})-(\\d{2})-(\\d{2})\\s+(\\w+)$"

        local id = tempRegexTrigger(pattern, function()
            send("match")
            snapshot = matches
        end, 1)

        feedTriggers("\n2025-11-17 Mudlet\n")

        assert.spy(send).was.called(1)
        assert.are.equal("2025-11-17 Mudlet", snapshot[1])
        assert.are.equal("2025", snapshot[2])
        assert.are.equal("11", snapshot[3])
        assert.are.equal("17", snapshot[4])
        assert.are.equal("Mudlet", snapshot[5])
        assert.is_nil(snapshot[6])
        killTrigger(id)
    end)

    -- alternation (|)
    it("supports alternation for multiple tokens", function()
        local send = spy.on(_G, "send")
        local snapshot = {}
        local pattern = "^Class: (mage|druid|cleric)$"

        local id = tempRegexTrigger(pattern, function()
            send("match")
            snapshot = matches
        end, 1)

        feedTriggers("\nClass: mage\n")

        assert.spy(send).was.called(1)
        assert.are.equal("Class: mage", snapshot[1])
        assert.are.equal("mage", snapshot[2])
        assert.is_nil(snapshot[3])
        killTrigger(id)
    end)

    -- optional groups (?: ... )?
    it("captures an optional group when present", function()
        local send = spy.on(_G, "send")
        local snapshot = {}
        local pattern = "^Name: (\\w+)(?:\\s+Class: (\\w+))?$"

        local id = tempRegexTrigger(pattern, function()
            send("match")
            snapshot = matches
        end, 1)

        feedTriggers("\nName: Tester Class: druid\n")

        assert.spy(send).was.called(1)
        assert.are.equal("Name: Tester Class: druid", snapshot[1])
        assert.are.equal("Tester", snapshot[2])
        assert.are.equal("druid", snapshot[3])
        assert.is_nil(snapshot[4])
        killTrigger(id)
    end)

    it("wont capture an optional group when not present", function()
        local send = spy.on(_G, "send")
        local snapshot = {}
        local pattern = "^Name: (\\w+)(?:\\s+Class: (\\w+))?$"

        local id = tempRegexTrigger(pattern, function()
            send("match")
            snapshot = matches
        end, 1)

        feedTriggers("\nName: Tester\n")

        assert.spy(send).was.called(1)
        assert.are.equal("Name: Tester", snapshot[1])
        assert.are.equal("Tester", snapshot[2])
        assert.is_nil(snapshot[3])
        killTrigger(id)
    end)    

    -- word boundaries (\b)
    it("uses \\b to ensure whole-word match", function()
        local send = spy.on(_G, "send")
        local snapshot = {}
        local pattern = "^.*\\bbad\\b.*$"

        local id = tempRegexTrigger(pattern, function()
            send("match")
            snapshot = matches
        end, 1)

        feedTriggers("\nthe big bad ogre\n")

        assert.spy(send).was.called(1)
        assert.are.equal("the big bad ogre", snapshot[1])
        assert.is_nil(snapshot[2])
        killTrigger(id)
    end)

    -- quantifiers and repeats (*, +, {m,n})
    it("supports greedy quantifiers and bounded repeats", function()
        local send = spy.on(_G, "send")
        local snapshot = {}
        local pattern = "^(https?://[^\\s]+)$"

        local id = tempRegexTrigger(pattern, function()
            send("match")
            snapshot = matches
        end, 1)

        feedTriggers("\nhttp://mudlet.org\n")

        assert.spy(send).was.called(1)
        assert.are.equal("http://mudlet.org", snapshot[1])
        assert.are.equal("http://mudlet.org", snapshot[2])
        assert.is_nil(snapshot[3])
        killTrigger(id)
    end)

    it("greedy captures the longest possible segment before backtracking", function()
        local send = spy.on(_G, "send")
        local snapshot = {}
        local pattern = "^(.*)\\s\\-\\s(.*)$"

        local id = tempRegexTrigger(pattern, function()
            send("match")
            snapshot = matches
        end, 1)

        feedTriggers("\nA - B - C\n")

        assert.spy(send).was.called(1)
        assert.is_table(snapshot)
        assert.are.equal("A - B - C", snapshot[1])
        assert.are.equal("A - B", snapshot[2])
        assert.are.equal("C", snapshot[3])
        assert.is_nil(snapshot[4])
        killTrigger(id)
    end)


    -- bounded repeats {m,n} on letters and digits
    it("matches with bounded repeats using {m,n}", function()
        local send = spy.on(_G, "send")
        local snapshot = {}
        local pattern = "^([A-Z]{2,4})\\-(\\d{3,5})$"

        local id = tempRegexTrigger(pattern, function()
            send("match")
            snapshot = matches
        end, 1)

        feedTriggers("\nABCD-12345\n")

        assert.spy(send).was.called(1)
        assert.is_table(snapshot)
        assert.are.equal("ABCD-12345", snapshot[1])
        assert.are.equal("ABCD",       snapshot[2])
        assert.are.equal("12345",      snapshot[3])
        assert.is_nil(snapshot[4])
        killTrigger(id)
    end)


    -- escaping special characters (literal . and ())
    it("matches literal parentheses and dots by escaping them", function()
        local send = spy.on(_G, "send")
        local snapshot = {}
        local pattern = "^File: ([\\w\\-]+)\\.(txt|log) \\(\\d+kb\\)$"

        local id = tempRegexTrigger(pattern, function()
            send("match")
            snapshot = matches
        end, 1)

        feedTriggers("\nFile: mylog-2025.log (10kb)\n")

        assert.spy(send).was.called(1)
        assert.are.equal("File: mylog-2025.log (10kb)", snapshot[1])
        assert.are.equal("mylog-2025", snapshot[2])
        assert.are.equal("log", snapshot[3])
        assert.is_nil(snapshot[4])
        killTrigger(id)
    end)

    -- case-insensitive flag (?i)
    it("matches case-insensitively using (?i)", function()
        local send = spy.on(_G, "send")
        local snapshot = {}
        local pattern = "(?i)^hello, (\\w+)$"

        local id = tempRegexTrigger(pattern, function()
            send("match")
            snapshot = matches
        end, 1)

        feedTriggers("\nHeLLo, Mudlet\n")

        assert.spy(send).was.called(1)
        assert.are.equal("HeLLo, Mudlet", snapshot[1])
        assert.are.equal("Mudlet", snapshot[2])
        assert.is_nil(snapshot[3])
        killTrigger(id)
    end)

    -- non-greedy capture (.+?)
    it("uses non-greedy capture to stop at first delimiter", function()
        local send = spy.on(_G, "send")
        local snapshot = {}
        local pattern = "^\\[(.+?)\\]\\s+(.*)$"

        local id = tempRegexTrigger(pattern, function()
            send("match")
            snapshot = matches
        end, 1)

        feedTriggers("\n[INFO] map loaded (0.5s)\n")

        assert.spy(send).was.called(1)
        assert.are.equal("[INFO] map loaded (0.5s)", snapshot[1])
        assert.are.equal("INFO", snapshot[2])
        assert.are.equal("map loaded (0.5s)", snapshot[3])
        assert.is_nil(snapshot[4])
        killTrigger(id)
    end)

    -- https://github.com/Mudlet/Mudlet/issues/8912
    -- selectCaptureGroup with named groups should select the actual capture position,
    -- not the first occurrence of the captured text in the line
    it("selectCaptureGroup selects correct position for named groups when captured text appears earlier in line", function()
        local selection
        local pattern = "^Hp: (?<chp>[0-9\\-]+)/(?<mhp>[0-9]+) Sp: (?<csp>[0-9\\-]+)/(?<msp>[0-9]+) Ep: (?<cep>[0-9\\-]+)/(?<mep>[0-9]+) Wght: (?<pwt>[0-9\\.]+)% Gold: (?<gld>[0-9]+) Algn: (?<align>[A-Za-z]+) Wpn: (?<wpn>W|H|N)$"

        local id = tempRegexTrigger(pattern, function()
            selectCaptureGroup("wpn")
            selection = getSelection()
            deselect()
        end, 1)

        feedTriggers("\nHp: 1451/1451 Sp: 6625/6625 Ep: 971/971 Wght: 14.1% Gold: 0 Algn: Angelic Wpn: H\n")

        assert.are.equal("H", selection)
        killTrigger(id)
    end)

    -- named capture groups populate matches table with both numeric and named keys
    it("named capture groups are accessible by name in matches table", function()
        local send = spy.on(_G, "send")
        local snapshot = {}
        local pattern = "^(?<first>\\w+) (?<second>\\w+)$"

        local id = tempRegexTrigger(pattern, function()
            send("match")
            snapshot = matches
        end, 1)

        feedTriggers("\nHello World\n")

        assert.spy(send).was.called(1)
        assert.are.equal("Hello World", snapshot[1])
        assert.are.equal("Hello", snapshot[2])
        assert.are.equal("World", snapshot[3])
        assert.are.equal("Hello", snapshot["first"])
        assert.are.equal("World", snapshot["second"])
        assert.is_nil(snapshot[4])
        killTrigger(id)
    end)

    -- selectCaptureGroup by name selects correct text
    it("selectCaptureGroup by name selects the right text", function()
        local selection_first, selection_second
        local pattern = "^(?<first>\\w+) (?<second>\\w+)$"

        local id = tempRegexTrigger(pattern, function()
            selectCaptureGroup("first")
            selection_first = getSelection()
            deselect()
            selectCaptureGroup("second")
            selection_second = getSelection()
            deselect()
        end, 1)

        feedTriggers("\nHello World\n")

        assert.are.equal("Hello", selection_first)
        assert.are.equal("World", selection_second)
        killTrigger(id)
    end)

    -- selectCaptureGroup returns -1 for non-existent named group
    it("selectCaptureGroup returns -1 for non-existent named group", function()
        local result
        local pattern = "^(?<name>\\w+)$"

        local id = tempRegexTrigger(pattern, function()
            result = selectCaptureGroup("nonexistent")
        end, 1)

        feedTriggers("\nMudlet\n")

        assert.are.equal(-1, result)
        killTrigger(id)
    end)

    -- named groups in alternation: non-participating group must not crash
    it("named groups in alternation don't crash when one group doesn't participate", function()
        local result_a, result_b
        local snapshot = {}
        local pattern = "^(?<a>cat)|(?<b>dog)$"

        local id = tempRegexTrigger(pattern, function()
            snapshot = matches
            result_a = selectCaptureGroup("a")
            result_b = selectCaptureGroup("b")
        end, 1)

        feedTriggers("\ncat\n")

        assert.are.equal("cat", snapshot["a"])
        assert.is_nil(snapshot["b"])
        assert.are_not.equal(-1, result_a)
        assert.are.equal(-1, result_b)
        killTrigger(id)
    end)

    -- named groups with repeated captured text elsewhere in the line
    it("selectCaptureGroup selects correct position when same word appears multiple times", function()
        local selection
        local pattern = "^(\\w+) said (?<last_word>\\w+)$"

        local id = tempRegexTrigger(pattern, function()
            selectCaptureGroup("last_word")
            selection = getSelection()
            deselect()
        end, 1)

        -- "hello" appears twice - selectCaptureGroup should pick the captured one (at the end)
        feedTriggers("\nhello said hello\n")

        assert.are.equal("hello", selection)
        killTrigger(id)
    end)

    -- mixed named and unnamed capture groups
    it("mixed named and unnamed groups both work in matches table", function()
        local send = spy.on(_G, "send")
        local snapshot = {}
        local pattern = "^(\\d+) (?<word>\\w+) (\\d+)$"

        local id = tempRegexTrigger(pattern, function()
            send("match")
            snapshot = matches
        end, 1)

        feedTriggers("\n42 hello 99\n")

        assert.spy(send).was.called(1)
        assert.are.equal("42 hello 99", snapshot[1])
        assert.are.equal("42", snapshot[2])
        assert.are.equal("hello", snapshot[3])
        assert.are.equal("99", snapshot[4])
        assert.are.equal("hello", snapshot["word"])
        assert.is_nil(snapshot[5])
        killTrigger(id)
    end)

    -- selectCaptureGroup with a number uses matches[] indexing:
    -- selectCaptureGroup(1) = matches[1] = full match
    -- selectCaptureGroup(2) = matches[2] = first capture group
    -- selectCaptureGroup by name selects the named group directly
    it("selectCaptureGroup by number uses matches[] indexing, by name selects named group", function()
        local sel_by_name, sel_by_number
        local pattern = "^Score: (?<score>\\d+) Level: (?<level>\\d+)$"

        local id = tempRegexTrigger(pattern, function()
            selectCaptureGroup("level")
            sel_by_name = getSelection()
            deselect()
            -- "level" is the 2nd capture group, so it's matches[3]
            selectCaptureGroup(3)
            sel_by_number = getSelection()
            deselect()
        end, 1)

        feedTriggers("\nScore: 100 Level: 50\n")

        assert.are.equal("50", sel_by_name)
        assert.are.equal("50", sel_by_number)
        killTrigger(id)
    end)

    -- no match
    it("doesnt falsely match a non matching line", function()
        local send = spy.on(_G, "send")
        local snapshot = {}
        local pattern = "^no match$"

        local id = tempRegexTrigger(pattern, function()
            send("match")
            snapshot = matches
        end, 1)

        feedTriggers("\nMudlet\n")

        assert.spy(send).was_not_called()
        killTrigger(id)
    end)    
end)
