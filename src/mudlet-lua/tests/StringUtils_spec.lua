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

    it("should not treat % as a format directive", function()
      assert.equals("100% sure", f("100% sure"))
      assert.equals("50% of 4 is 2", f("50% of 4 is {4/2}"))
    end)
  end)
end)
