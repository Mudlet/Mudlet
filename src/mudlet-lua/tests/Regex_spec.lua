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
