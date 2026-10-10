describe("Tests StringUtils.lua functions", function()
  describe("Tests the functionality of string.cut", function()
    it("should return the same string if it is <= maxLen", function()
      local testString = "test"
      assert.equals(testString, string.cut(testString, testString:len()))
      assert.equals(testString, testString:cut(testString:len()))
      assert.equals(testString, string.cut(testString, testString:len() + 1 ))
      assert.equals(testString, testString:cut(testString:len() + 1 ))
    end)

    it("should return a string of length maxLen if it is given a string longer than that", function()
      local testString = "This is a test of the emergency string cutting system"
      local expected = "This is a "
      local expectedLength = 10
      local actual = string.cut(testString, expectedLength)
      local actualLength = actual:len()
      assert.equals(expected, actual)
      assert.equals(expectedLength, actualLength)
      actual = testString:cut(expectedLength)
      actualLength = actual:len()
      assert.equals(expectedLength, actualLength)
    end)

    it("should return an empty string when asked for no characters at all", function()
      assert.equals("", ("This is a test"):cut(0))
      assert.equals("", (""):cut(0))
    end)
  end)

  describe("Tests the functionality of string.enclose", function()
    it("should return [[]] is empty string is given", function()
      assert.equals("[[]]", string.enclose(""))
      local testString = ""
      assert.equals("[[]]", testString:enclose())
    end)

    it("should return the string str wrapped in [[]]", function()
      local s = "This is a test"
      local expected = '[[This is a test]]'
      local actual = string.enclose(s)
      assert.equals(expected, actual)
    end)

    it("should detect and insert = up to maxlevel to avoid accidental string closure", function()
      local s = "[[This is a [=[test]=] of string.enclose]]"
      local expected = "[==[" .. s .. "]==]"
      local actual = string.enclose(s, 3)
      assert.equals(expected, actual)
    end)

    it("should error if maxlevel is not high enough to properly wrap the string", function()
      local s = "[=[[[This is a test]]]=]"
      local errfn = function() string.enclose(s,1) end
      assert.has_error(errfn, "error: maxlevel too low, 1")
    end)

    it("should need a maxlevel above the level it actually settles on", function()
      -- the ceiling is checked before the level is tried, so a string that needs
      -- one = still fails at maxlevel 1 and only succeeds from 2 upwards
      assert.has_error(function() string.enclose("[[x]]", 1) end, "error: maxlevel too low, 1")
      assert.equals("[=[[[x]]]=]", string.enclose("[[x]]", 2))
      assert.has_error(function() string.enclose("plain", 0) end, "error: maxlevel too low, 0")
    end)
  end)

  describe("Tests the functionality of string.ends", function()
    it("should return true if str ends in suffix", function()
      local s = "This is a test"
      local suffix = "a test"
      assert.is_true(string.ends(s, suffix))
      assert.is_true(s:ends(suffix))
      s = "Of the emergency broadcasting system"
      suffix = "system"
      assert.is_true(string.ends(s, suffix))
    end)
    it("should return false if str does not end in suffix", function()
      local s = "This is a test"
      local suffix = "system"
      assert.is_false(string.ends(s, suffix))
    end)

    it("should return true for an empty suffix", function()
      assert.is_true(("This is a test"):ends(""))
    end)

    it("should return false when the suffix is longer than the string", function()
      assert.is_false(("hi"):ends("this is far too long"))
    end)
  end)

  describe("Tests the functionality of string.genNocasePattern", function()
    it("should create a case insensitive lua pattern based on str", function()
      local str = "123abc"
      local expected = "123[aA][bB][cC]"
      local actual = string.genNocasePattern(str)
      assert.equals(expected, actual)
      local actual = str:genNocasePattern()
      assert.equals(expected, actual)
    end)

    it("should be able to match the string it was generated from", function()
      local str = "123abc"
      local pattern = string.genNocasePattern(str)
      assert.is_truthy(str:find(pattern))
    end)

    it("should return only the pattern, not gsub's replacement count", function()
      -- returning gsub directly would hand a second value to every caller, and
      -- string.format("%s", str:genNocasePattern()) would still look right
      assert.equals(1, select("#", ("abc"):genNocasePattern()))
    end)

    it("should leave bytes that are not ASCII letters alone", function()
      -- %a is byte-wise, so the two bytes of á cannot be case-folded and have to
      -- survive untouched or the pattern stops matching its own source string
      local pattern = ("ábc"):genNocasePattern()
      assert.equals("á[bB][cC]", pattern)
      assert.is_truthy(("ábc"):find(pattern))
    end)
  end)

  describe("Tests the functionality of string.findPattern", function()
    it("should return the first match of pattern in str", function()
      local str = "This is test 2"
      local pattern = "test %d"
      local expected = "test 2"
      local actual = string.findPattern(str, pattern)
      assert.equals(expected, actual)
      local actual = str:findPattern(pattern)
      assert.equals(expected, actual)
    end)

    it("should return nil if there is no match", function()
      local str = "This is the test"
      local pattern = "is a"
      assert.is_nil(string.findPattern(str, pattern))
    end)
  end)

  describe("Tests the functionality of string.split", function()
    it("should return a table which contain the pieces of str, cut by delimiter", function()
      local str = "This is,a comma,separated string,with stuff in it"
      local delimiter = ","
      local expected = {
        "This is",
        "a comma",
        "separated string",
        "with stuff in it"
      }
      local actual = str:split(delimiter)
      assert.same(expected, actual)
      local actual = string.split(str, delimiter)
      assert.same(expected, actual)
    end)

    it("should return a table with one item being the original string", function()
      local str = "This is a test"
      local expected = { str }
      local actual = str:split(":")
      assert.same(expected, actual)
    end)

    it("should default to splitting on a space", function()
      local str = "This is a test"
      local expected = { "This", "is", "a", "test" }
      local actual = str:split()
      assert.same(expected, actual)
    end)

    it("should return a table with the characters that make up the string if empty string is used for the delimiter", function()
      local str = "This is a test"
      local expected = {"T", "h", "i", "s", " ", "i", "s", " ", "a", " ", "t", "e", "s", "t"}
      local actual = str:split("")
      assert.same(expected, actual)
    end)

    it("should split on a multi-character delimiter", function()
      local str = "alpha::beta::gamma"
      local expected = { "alpha", "beta", "gamma" }
      assert.same(expected, str:split("::"))
    end)

    it("should treat the delimiter as a Lua pattern, not a plain string", function()
      -- '.' is the 'any character' pattern, so it does not split on literal dots;
      -- the dot must be escaped to split on real dots.
      assert.same({ "1", "2", "3" }, ("1.2.3"):split("%."))
      assert.are_not.same({ "1", "2", "3" }, ("1.2.3"):split("."))
    end)

    it("should produce empty leading and trailing segments when the delimiter is at the edges", function()
      local str = ",a,b,"
      local expected = { "", "a", "b", "" }
      assert.same(expected, str:split(","))
    end)
  end)

  describe("Tests the functionality of string.starts", function()
    it("should return true if str starts with prefix", function()
      local str = "This is a test"
      assert.is_true(str:starts("This"))
      assert.is_true(string.starts(str, "This"))
    end)
    it("should return false if str does not start with prefix", function()
      local str = "This is a test"
      assert.is_false(str:starts("Elephant"))
    end)

    it("should return true for an empty prefix", function()
      assert.is_true(("This is a test"):starts(""))
    end)

    it("should return true when the prefix is the whole string", function()
      local str = "This is a test"
      assert.is_true(str:starts(str))
    end)
  end)

  describe("Tests the functionality of string.title", function()
    it("should return the string with the first letter capitalized", function()
      local str = "this"
      local expected = "This"
      local actual = str:title()
      assert.equals(expected, actual)
    end)

    it("should return the original string if the first letter is already capitalized", function()
      local str = "This"
      local actual = str:title()
      assert.equals(str, actual)
    end)

    it("should error if given something other than a string", function()
      local str = {}
      local errfn = function() string.title(str) end
      assert.has_error(errfn, "string.title: bad argument #1 type (string to title as string expected, got table!)")
    end)

    it("should return an empty string unchanged", function()
      assert.equals("", string.title(""))
    end)

    it("should leave a string that does not start with a lowercase letter unchanged", function()
      assert.equals("123abc", string.title("123abc"))
    end)

    it("should return only the titled string, not gsub's replacement count", function()
      assert.equals(1, select("#", ("abc"):title()))
      assert.equals(1, select("#", ("Abc"):title()))
    end)

    it("should only capitalise the first letter, not every word", function()
      assert.equals("This is a test", ("this is a test"):title())
    end)
  end)

  describe("Tests the functionality of string.trim", function()
    it("should return str with all spaces stripped from the beginning and end", function()
      local str = "    this is a test      "
      local expected = "this is a test"
      local actual = str:trim()
      assert.equals(expected, actual)
      actual = string.trim(str)
      assert.equals(expected, actual)
    end)

    it("should return whatever you pass in if it's falsey", function()
      local str = false
      assert.equals(str, string.trim(str))
      str = nil
      assert.equals(str, string.trim(str))
    end)

    it("should return the same string if str has no spaces at front or back", function()
      local str = "This is a test"
      assert.equals(str, string.trim(str))
      assert.equals(str, str:trim())
    end)

    it("should strip leading and trailing tabs and newlines, not just spaces", function()
      assert.equals("this is a test", ("\t\n  this is a test \n\t"):trim())
    end)

    it("should leave whitespace inside the string alone", function()
      assert.equals("a  b", ("  a  b  "):trim())
      assert.equals("", ("   "):trim())
    end)

    it("should keep long runs of whitespace inside the string", function()
      local inner = "HP: 100" .. string.rep(" ", 200) .. "MP:\t\t50" .. string.rep(" ", 200) .. "EP: 9"
      assert.equals(inner, ("   " .. inner .. string.rep(" ", 300)):trim())
      assert.equals("", (string.rep(" \t\n", 500)):trim())
      assert.equals("", (""):trim())
      assert.equals("x", (string.rep(" ", 500) .. "x"):trim())
    end)

    it("should trim a number the way it trims its string form", function()
      assert.equals("42", string.trim(42))
    end)

    it("should return only the trimmed string, not gsub's replacement count", function()
      assert.equals(1, select("#", ("  a  "):trim()))
      assert.equals(1, select("#", ("a"):trim()))
    end)
  end)

  describe("Tests the functionality of string.patternEscape", function()
    it("Should escape special characters in simple cases", function()
      local replacements = {
        ["%"] = "%%",
        ["^"] = "%^",
        ["$"] = "%$",
        ["("] = "%(",
        [")"] = "%)",
        ["["] = "%[",
        ["]"] = "%]",
        ["."] = "%.",
        ["*"] = "%*",
        ["+"] = "%+",
        ["-"] = "%-",
        ["?"] = "%?",
      }
      for original, replacement in pairs(replacements) do
        assert.equals(replacement, string.patternEscape(original))
      end
    end)

    it("Should escape special characters in some more complicated cases too", function()
      local replacements = {
        ["https://fern-ahead-jelly.glitch.me/time/"] = "https://fern%-ahead%-jelly%.glitch%.me/time/",
        ["75% of things to-be have been"] = "75%% of things to%-be have been",
        ["^%d%a$"] = "%^%%d%%a%$",
      }
      for original, replacement in pairs(replacements) do
        assert.equals(replacement, string.patternEscape(original))
      end
    end)

    -- the twelve pattern magic characters gain a "%" in front, every other
    -- byte - other punctuation, control characters, NUL and bytes >= 0x80 -
    -- passes through untouched
    local magic = "%^$()[].*+-?"
    local function expectedEscape(str)
      local out = {}
      for i = 1, #str do
        local c = str:sub(i, i)
        out[#out + 1] = magic:find(c, 1, true) and ("%" .. c) or c
      end
      return table.concat(out)
    end

    it("Should escape exactly the magic characters among all 256 byte values", function()
      for byte = 0, 255 do
        local c = string.char(byte)
        assert.equals(expectedEscape(c), string.patternEscape(c), "byte " .. byte)
        assert.equals(expectedEscape("a" .. c .. "b" .. c), string.patternEscape("a" .. c .. "b" .. c), "byte " .. byte)
      end
      local all = {}
      for byte = 0, 255 do
        all[#all + 1] = string.char(byte)
      end
      all = table.concat(all)
      assert.equals(expectedEscape(all), string.patternEscape(all))
    end)

    it("Should leave multibyte UTF-8 text alone", function()
      assert.equals("Café %(été%) 👊 %[ü%]", string.patternEscape("Café (été) 👊 [ü]"))
      assert.equals("naïve %- 日本語%.", string.patternEscape("naïve - 日本語."))
    end)

    it("Should return a pattern that finds the original text literally", function()
      for _, original in ipairs({"a.b", "[x]", "100%", "^start$", "(a+b)*c-d?", "Café (été) 👊"}) do
        local escaped = string.patternEscape(original)
        assert.are.same({1, #original}, {string.find(original, escaped)})
        assert.equals(original, string.match("xx" .. original .. "yy", escaped))
      end
    end)

    it("Should return one value, and an empty string for an empty string", function()
      assert.equals(1, select("#", string.patternEscape("a.b")))
      assert.equals(1, select("#", ("a.b"):patternEscape()))
      assert.equals("", string.patternEscape(""))
    end)

    it("Should raise an error for a non-string argument", function()
      for _, bad in ipairs({5, {}, true}) do
        local ok, err = pcall(string.patternEscape, bad)
        assert.is_false(ok)
        assert.is_truthy(tostring(err):find("string.patternEscape: bad argument #1 type", 1, true), tostring(err))
      end
      assert.has_error(function() string.patternEscape() end)
    end)
  end)

  describe("Tests the functionality of utf8.patternEscape", function()
    it("Should escape special characters in simple cases", function()
      local replacements = {
        ["%"] = "%%",
        ["^"] = "%^",
        ["$"] = "%$",
        ["("] = "%(",
        [")"] = "%)",
        ["["] = "%[",
        ["]"] = "%]",
        ["."] = "%.",
        ["*"] = "%*",
        ["+"] = "%+",
        ["-"] = "%-",
        ["?"] = "%?",
      }
      for original, replacement in pairs(replacements) do
        assert.equals(replacement, utf8.patternEscape(original))
      end
    end)

    it("Should escape special characters in some more complicated cases too", function()
      local replacements = {
        ["https://fern-ahead-jelly.glitch.me/time/"] = "https://fern%-ahead%-jelly%.glitch%.me/time/",
        ["75% of things to-be have been 👊"] = "75%% of things to%-be have been 👊",
        ["^%d%a$"] = "%^%%d%%a%$",
      }
      for original, replacement in pairs(replacements) do
        assert.equals(replacement, utf8.patternEscape(original))
      end
    end)

    it("Should escape exactly the magic characters among all ASCII characters", function()
      local magic = "%^$()[].*+-?"
      for byte = 0, 127 do
        local c = string.char(byte)
        local expected = magic:find(c, 1, true) and ("%" .. c) or c
        assert.equals(expected, utf8.patternEscape(c), "byte " .. byte)
        assert.equals("é" .. expected .. "👊" .. expected, utf8.patternEscape("é" .. c .. "👊" .. c), "byte " .. byte)
      end
    end)

    it("Should leave multibyte characters whole", function()
      assert.equals("naïve %- 日本語%. %(👊%)", utf8.patternEscape("naïve - 日本語. (👊)"))
      local escaped = utf8.patternEscape("Café (été) [ü]")
      assert.equals("Café %(été%) %[ü%]", escaped)
      assert.equals("Café (été) [ü]", utf8.match("xx Café (été) [ü] yy", escaped))
    end)

    it("Should return one value, and an empty string for an empty string", function()
      assert.equals(1, select("#", utf8.patternEscape("a.b")))
      assert.equals("", utf8.patternEscape(""))
    end)

    it("Should raise an error for a non-string argument", function()
      for _, bad in ipairs({5, {}, true}) do
        local ok, err = pcall(utf8.patternEscape, bad)
        assert.is_false(ok)
        assert.is_truthy(tostring(err):find("utf8.patternEscape: bad argument #1 type", 1, true), tostring(err))
      end
    end)
  end)

  describe("Tests the functionality of f", function()
    it("should return a string with no interpolation as itself", function()
      local str = "This is a test"
      local expected = str
      local actual = f(str)
      assert.equals(expected, actual)
    end)

    it("should return a string with simple interpolation", function()
      local str = "This is a {'test'}"
      local expected = "This is a test"
      local actual = f(str)
      assert.equals(expected, actual)
    end)

    it("should execute simple expressions within the interpolation characters", function()
      local str = "2 + 2 = {2+2}"
      local expected = "2 + 2 = 4"
      local actual = f(str)
      assert.equals(expected, actual)
    end)

    it("should be able to interpolate local variables", function()
      local str = "The secret sauce is {secret}"
      local secret = "well known"
      local expected = "The secret sauce is well known"
      local actual = f(str)
      assert.equals(expected, actual)
    end)

    it("should evaluate more complex expressions/functions in interpolation", function()
      local testFunc = function(msg)
        return msg:title()
      end
      local mesg = "sir"
      local str = "This is just a test, good {testFunc(mesg)}"
      local expected = "This is just a test, good Sir"
      local actual = f(str)
      assert.equals(expected, actual)
    end)

    it("should be able to handle two or more interpolations in a single string", function()
      local str = "This is a {test}. Do make sure to check {2+2} your belongings. {getMudletVersion('string')}"
      local test = "complete success"
      local expected = "This is a complete success. Do make sure to check 4 your belongings. " .. getMudletVersion('string')
      local actual = f(str)
      assert.equals(expected, actual)
    end)

    it("should error when passed in anything but a string", function()
      assert.has_error(function() f(true) end, "f: bad argument #1 type (str as string expected, got boolean)")
    end)

    it("should raise the compile error of an expression that is not valid Lua", function()
      -- swallowing this would silently drop the whole interpolation from the
      -- string, which is far harder to notice than a raised error
      assert.error_matches(function() f("{1+}") end, "unexpected symbol")
      assert.error_matches(function() f("{ this is not lua }") end, "expected near")
    end)

    it("should interpolate a global and render a missing name as nil", function()
      _G.stringUtilsSpecGlobal = "seen"
      local interpolated = f("a {stringUtilsSpecGlobal} b")
      _G.stringUtilsSpecGlobal = nil
      assert.equals("a seen b", interpolated)
      assert.equals("nil", f("{stringUtilsSpecGlobal}"))
    end)

    it("should read the current value of a name each time the same string is used", function()
      for i = 1, 3 do
        assert.equals("i is " .. i, f("i is {i}"))
      end
    end)

    it("should give a nested f the locals in scope where it runs", function()
      local outerName = "outer"
      local function inner()
        local innerName = "inner"
        -- not a tail call, which would take this frame and its locals off the stack
        local result = f("{innerName}/{outerName}")
        return result
      end
      assert.equals("inner/outer", inner())
      assert.equals("[inner/outer] outer", f("[{inner()}] {outerName}"))
    end)

    it("should interpolate a variable named like one of f's own locals", function()
      do
        local lookup, outer_env, code = "local lookup", "local outer_env", "local code"
        assert.equals("local lookup/local outer_env/local code", f("{lookup}/{outer_env}/{code}"))
      end

      local was = rawget(_G, "lookup")
      _G.lookup = {x = 42}
      local ok, result = pcall(f, "{lookup.x}")
      _G.lookup = was
      assert.is_true(ok, tostring(result))
      assert.equals("42", result)
    end)

    it("should not let an outer f's locals shadow those of a function it calls", function()
      local code, block, exp_env = "mine", "my block", "my env"
      local function inner()
        return f("{code}/{block}/{exp_env}")
      end
      assert.equals("[mine/my block/my env]", f("[{inner()}]"))
    end)

    -- a function made in an expression writes its globals into that
    -- expression's own environment, which nothing else may see
    it("should not let a global assigned inside one expression leak into the next", function()
      assert.equals("1", f("{(function() fstringSpecLeak = 1 return fstringSpecLeak end)()}"))
      assert.is_nil(rawget(_G, "fstringSpecLeak"))
      assert.equals("nil", f("{fstringSpecLeak}"))
    end)

    it("should not let an expression that raised leave a global behind", function()
      assert.is_false(pcall(f, "{(function() fstringSpecStale = 'stale' error('boom') end)()}"))
      local fstringSpecStale = "fresh"
      assert.equals("fresh", f("{fstringSpecStale}"))
    end)

    it("should interpolate several plain names, with text around and between them", function()
      local first, second = "one", 2
      local result = f("[{first}]{second}{first} and {second}!")
      assert.equals("[one]2one and 2!", result)
    end)

    it("should resolve several names in one string from locals, globals and nil", function()
      _G.fstringSpecMixed = "global"
      local fromLocal = "local"
      local result = f("{fromLocal} {fstringSpecMixed} {fstringSpecAbsent}")
      _G.fstringSpecMixed = nil
      assert.equals("local global nil", result)
    end)

    it("should render a local holding nil as nil even when a global shares its name", function()
      _G.fstringSpecShadowed = "global"
      local fstringSpecShadowed = nil
      local result = f("{fstringSpecShadowed}")
      _G.fstringSpecShadowed = nil
      assert.equals("nil", result)
    end)

    it("should render locals holding nil and false as such in a template of several names", function()
      _G.fstringSpecShadowedPair = "global"
      local fstringSpecShadowedPair, other = nil, false
      local result = f("{fstringSpecShadowedPair}{other}")
      _G.fstringSpecShadowedPair = nil
      assert.equals("nilfalse", result)
    end)

    it("should take a name's locals from the caller's callers as well", function()
      local fromOuter = "outer"
      local function middle()
        local fromMiddle = "middle"
        local function inner()
          local result = f("{fromMiddle} {fromOuter}")
          return result
        end
        return (inner())
      end
      assert.equals("middle outer", middle())
    end)

    it("should find a single name in a caller's caller", function()
      local fromOuter = "outer"
      local function inner()
        local result = f("{fromOuter}")
        return result
      end
      local function middle()
        local result = inner()
        return result
      end
      assert.equals("outer", middle())
    end)

    it("should let a function inside an expression read the caller's locals, whatever f calls its own", function()
      local i, out, code, part = "i", "out", "code", "part"
      local function pick(name)
        return name
      end
      assert.equals("i", f("{(function() return i end)()}"))
      assert.equals("out", f("{(function() return out end)()}"))
      assert.equals("code", f("{(function() return code end)()}"))
      assert.equals("part", f("{(function() return part end)()}"))
      assert.equals("i", f("{pick((function() return i end)())}"))
    end)

    it("should read each name when its block runs, so an earlier block can change it", function()
      local n, m = 0, "m"
      local function inc()
        n = n + 1
        return n
      end
      local result = f("{inc()} {n} {m}")
      assert.equals("1 1 m", result)
    end)

    it("should not let a nested f see the outer f's internal locals", function()
      local i, out, parts = "mine", "mine2", "mine3"
      local result = f("{f('{i}')}|{f('{out}')}|{f('{parts}')}")
      assert.equals("mine|mine2|mine3", result)
    end)

    it("should reuse a template with fresh locals on every call", function()
      local results = {}
      for _, v in ipairs({"a", "b", "c"}) do
        local item = v
        local n = #results
        results[n + 1] = f("{item}-{item}/{n}")
      end
      assert.same({"a-a/0", "b-b/1", "c-c/2"}, results)
    end)

    it("should render a name repeated in one template", function()
      local a = 1
      local result = f("{a}{a}{a}")
      assert.equals("111", result)
    end)

    it("should not let a nested f with several names see the outer f's internal locals", function()
      local i, out, parts = "mine", "mine2", "mine3"
      local result = f("{f('{i}{out}')}|{f('{parts}{i}')}")
      assert.equals("minemine2|mine3mine", result)
    end)

    it("should not let a nested f read the outer f's template through its parameter", function()
      local result = f("{f('{supersecretstringvariablenocollision}')}")
      assert.equals("nil", result)
    end)

    it("should see a local that a __tostring changes before a later name is read", function()
      local n = 0
      local counter = setmetatable({}, { __tostring = function()
        n = n + 1
        return "o"
      end })
      local result = f("{counter}{n}")
      assert.equals("o1", result)
    end)

    it("should read the caller's locals from a callback that other functions call", function()
      local function each(t, fn)
        local r = {}
        for i, v in ipairs(t) do
          r[i] = fn(v)
        end
        return r
      end
      local function helper(t, fn)
        local factor = "helper's"
        local r = each(t, fn)
        return r
      end
      local factor = 3
      local result = f("{table.concat(helper({1, 2}, function(x) return x * factor end), ',')}")
      assert.equals("3,6", result)
    end)

    it("should take each name from the innermost frame that has it", function()
      local function outer()
        local x, y = "outer-x", "outer-y"
        local function inner()
          local x = "inner-x"
          local result = f("{x}{y}")
          return result
        end
        local result = inner()
        return result
      end
      assert.equals("inner-xouter-y", outer())
    end)

    it("should prefer the innermost local when a name exists at several depths", function()
      local x, y = "outer-x", "outer-y"
      local function inner()
        local x, y = "inner-x", "inner-y"
        local both = f("{x}{y}")
        local single = f("{x}{x}")
        return both, single
      end
      local both, single = inner()
      assert.equals("inner-xinner-y", both)
      assert.equals("inner-xinner-x", single)
    end)

    it("should mix plain names and expressions, with text after the last block", function()
      local a, b = 1, 2
      local result = f("{a}+{b}={a + b} {a}{(b * 2)}-tail")
      assert.equals("1+2=3 14-tail", result)
    end)

    it("should leave a template with no complete block untouched", function()
      assert.equals("a{b", f("a{b"))
    end)

    it("should read line and matches through a plain name", function()
      _G.line = "hello"
      local plain = f("{line}")
      local shadowed
      do
        local line = "local"
        shadowed = f("{line}")
      end
      _G.line = nil
      assert.equals("hello", plain)
      assert.equals("local", shadowed)
    end)

    it("should read a plain name's global from the environment f itself is given", function()
      local sandbox = setmetatable({fstringSpecPlainSandboxed = "inside"}, {__index = _G})
      local original = getfenv(f)
      setfenv(f, sandbox)
      local ok, plain, pair = pcall(function()
        return f("{fstringSpecPlainSandboxed}"), f("{fstringSpecPlainSandboxed}{fstringSpecPlainSandboxed}")
      end)
      setfenv(f, original)
      assert.is_true(ok)
      assert.equals("inside", plain)
      assert.equals("insideinside", pair)
    end)

    it("should treat a Lua keyword in braces as an expression, not a name", function()
      assert.equals("nil true false", f("{nil} {true} {false}"))
    end)

    it("should evaluate a block whose expression holds braces of its own", function()
      assert.equals("2", f("{({1, 2})[2]}"))
    end)

    it("should keep an expression's globals through an f nested inside it", function()
      assert.equals("1", f("{(function() fstringSpecNested = 1 local _ = f('{2}') return fstringSpecNested end)()}"))
    end)

    it("should keep the globals of a function an expression made for later", function()
      local store = {}
      assert.equals("made", f("{(function() store.bump = function() fstringSpecCount = (fstringSpecCount or 0) + 1 return fstringSpecCount end return 'made' end)()}"))
      assert.equals(1, store.bump())
      assert.equals(2, store.bump())
      local fstringSpecCount = "local"
      assert.equals("local", f("{fstringSpecCount}"))
      assert.equals(3, store.bump())
    end)

    it("should not let a function an expression calls leave a global in its environment", function()
      local setter = function(k, v)
        getfenv(2)[k] = v
        return "set"
      end
      -- concatenated so the call is not a tail call, which would leave getfenv(2) nothing to find
      assert.equals("set", f("{setter('fstringSpecPlanted', 'planted') .. ''}"))
      local fstringSpecPlanted = "local"
      assert.equals("local", f("{fstringSpecPlanted}"))
    end)

    it("should keep what a helper wrote into an expression's environment through a nested f", function()
      local setter = function(k, v)
        getfenv(2)[k] = v
        return ""
      end
      local nested = function()
        return f("{1}")
      end
      assert.equals("1kept", f("{setter('fstringSpecHelperWrite', 'kept') .. nested() .. fstringSpecHelperWrite}"))
    end)

    it("should give every evaluation an environment whose lookup a helper cannot spoil for the next", function()
      local spoiler = function()
        setmetatable(getfenv(2), {})
        return "spoilt"
      end
      assert.equals("spoilt", f("{spoiler() .. ''}"))
      local hp = 42
      assert.equals("42", f("{hp}"))
    end)

    it("should read globals from the environment f itself is given", function()
      local sandbox = setmetatable({fstringSpecSandboxed = "inside"}, {__index = _G})
      local original = getfenv(f)
      _G.fstringSpecOutside = "outside"
      setfenv(f, sandbox)
      local ok, inside, outside = pcall(function()
        return f("{fstringSpecSandboxed}"), f("{fstringSpecOutside}")
      end)
      setfenv(f, original)
      _G.fstringSpecOutside = nil
      assert.is_true(ok, tostring(inside))
      assert.equals("inside", inside)
      assert.equals("nil", outside)
    end)

    it("should not treat % as a format directive", function()
      assert.equals("100% sure", f("100% sure"))
      assert.equals("50% of 4 is 2", f("50% of 4 is {4/2}"))
    end)
  end)
end)
