describe("Tests DateTime.lua functions", function()

  describe("Tests the functionality of shms", function()
    it("should seconds as hh, mm, ss when called without a second parameter", function()
      local expected = { '00', '01', '00' }
      assert.are.same(expected, {shms(60)})
      local expected = { '01', '32', '15' }
      assert.are.same(expected, {shms(5535)})
    end)

    it("should cecho the information out if the second parameter is truthy", function()
      local seconds = 5535
      local expected = { '01', '32', '15' }
      local expectedString = string.format("<green>%s <grey>seconds converts to: <green>%s<white>h,<green> %s<white>m <grey>and<green> %s<white>s.", seconds, expected[1], expected[2], expected[3])
      -- quick and dirty stub
      _G.oldcecho = _G.cecho
      _G.cecho = function() end
      local cecho = spy.on(_G, "cecho")
      shms(seconds, true)
      assert.spy(cecho).was.called(1)
      assert.spy(cecho).was.called_with(expectedString)
      -- undo my sins
      _G.cecho = _G.oldcecho
      _G.oldcecho = nil
    end)

    it("should raise its documented assertion for something that is not a number", function()
      local message = "Please supply a valid number"
      assert.error_matches(function() shms("not a number") end, message)
      assert.error_matches(function() shms(nil) end, message)
      assert.error_matches(function() shms({}) end, message)
    end)

    it("should accept a number written as a string", function()
      assert.are.same({'01', '32', '15'}, {shms("5535")})
    end)

    it("should keep counting hours past 24 rather than wrapping to a new day", function()
      -- an uptime or a fight timer is not a clock, so 25 hours has to stay 25
      assert.are.same({'25', '01', '01'}, {shms(90061)})
    end)

    it("should truncate a fractional second rather than rounding it up", function()
      assert.are.same({'00', '01', '01'}, {shms(61.9)})
    end)
  end)

  describe("Tests datetime:parse", function()
    it("parses the default ISO format into a date table", function()
      local dt = datetime:parse("2025-06-15 19:34:42")
      assert.are.equal(2025, dt.year)
      assert.are.equal(6, dt.month)
      assert.are.equal(15, dt.day)
      assert.are.equal("number", type(dt.day))
      assert.are.equal(19, dt.hour)
      assert.are.equal(34, dt.min)
      assert.are.equal(42, dt.sec)
    end)

    it("parses a full month name with %B", function()
      local dt = datetime:parse("June 15, 2025", "^%B %d, %Y$")
      assert.are.equal(2025, dt.year)
      assert.are.equal(6, dt.month)
      assert.are.equal(15, dt.day)
    end)

    it("parses an abbreviated month name with %b", function()
      local dt = datetime:parse("Jun 15 2025", "^%b %d %Y$")
      assert.are.equal(6, dt.month)
      assert.are.equal(15, dt.day)
    end)

    it("parses month names case-insensitively", function()
      local dt = datetime:parse("JUNE 15, 2025", "^%B %d, %Y$")
      assert.are.equal(6, dt.month)
    end)

    it("expands a 2-digit year with %y into the 2000s", function()
      local dt = datetime:parse("25-06-15", "^%y-%m-%d$")
      assert.are.equal(2025, dt.year)
      assert.are.equal(6, dt.month)
    end)

    it("converts 12-hour PM times to 24-hour", function()
      local dt = datetime:parse("2025-06-15 01:30:00 PM", "^%Y-%m-%d %I:%M:%S %p$")
      assert.are.equal(13, dt.hour)
      assert.are.equal(30, dt.min)
    end)

    it("keeps 12-hour AM times in the morning", function()
      local dt = datetime:parse("2025-06-15 07:30:00 AM", "^%Y-%m-%d %I:%M:%S %p$")
      assert.are.equal(7, dt.hour)
    end)

    it("treats 12 PM as noon, hour 12, not hour 24", function()
      local dt = datetime:parse("2020-01-01 12:30 PM", "^%Y-%m-%d %I:%M %p$")
      assert.are.equal(12, dt.hour)
      assert.are.equal(30, dt.min)
    end)

    it("treats 12 AM as midnight, hour 0", function()
      local dt = datetime:parse("2020-01-01 12:30 AM", "^%Y-%m-%d %I:%M %p$")
      assert.are.equal(0, dt.hour)
      assert.are.equal(30, dt.min)
    end)

    it("treats a lowercase pm the same as an uppercase PM", function()
      local dt = datetime:parse("2020-01-01 01:00 pm", "^%Y-%m-%d %I:%M %p$")
      assert.are.equal(13, dt.hour)
    end)

    it("treats a lowercase am the same as an uppercase AM", function()
      local dt = datetime:parse("2020-01-01 12:00 am", "^%Y-%m-%d %I:%M %p$")
      assert.are.equal(0, dt.hour)
    end)

    it("errors when %I is used without %p", function()
      local ok, err = pcall(function()
        datetime:parse("2025-06-15 07:30:00", "^%Y-%m-%d %I:%M:%S$")
      end)
      assert.is_false(ok)
      assert.is_true(string.find(err, "12-hour", 1, true) ~= nil)
    end)

    it("returns nil when the source does not match the format", function()
      assert.is_nil(datetime:parse("not a date"))
      assert.is_nil(datetime:parse("2025-06-15", "^%Y-%m-%d %H:%M:%S$"))
    end)

    it("returns a Unix epoch when as_epoch is true", function()
      local epoch = datetime:parse("2025-06-15 12:30:45", nil, true)
      assert.are.equal("number", type(epoch))
      local back = os.date("*t", epoch)
      assert.are.equal(2025, back.year)
      assert.are.equal(6, back.month)
      assert.are.equal(15, back.day)
      assert.are.equal(12, back.hour)
      assert.are.equal(30, back.min)
      assert.are.equal(45, back.sec)
    end)

    -- the isdst lookup calls os.time() on the half built date table before
    -- anything has checked that the format supplied a day
    pending("datetime:parse handles a format with no date part - '05:06:07' with '^%H:%M:%S$' raises \"field 'day' missing in date table\" - issue #10415")
  end)

  describe("Tests datetime:parse round-trips with string formatting", function()
    it("round-trips an ISO timestamp through os.date", function()
      local s = "2025-06-15 12:30:45"
      local epoch = datetime:parse(s, nil, true)
      assert.are.equal(s, os.date("%Y-%m-%d %H:%M:%S", epoch))
    end)

    it("round-trips a custom slash/colon format through os.date", function()
      local s = "06/15/2025 08:05"
      local epoch = datetime:parse(s, "^%m/%d/%Y %H:%M$", true)
      assert.are.equal(s, os.date("%m/%d/%Y %H:%M", epoch))
    end)
  end)

  describe("Tests datetime:calculate_UTCdiff", function()
    it("returns a numeric offset within the valid timezone range", function()
      local diff = datetime:calculate_UTCdiff(1718452245)
      assert.are.equal("number", type(diff))
      assert.is_true(diff >= -14 * 3600 and diff <= 14 * 3600)
      -- every real timezone offset is a whole number of 15-minute steps
      assert.are.equal(0, diff % 900)
    end)

    it("is deterministic for the same instant", function()
      local t = 1718452245
      assert.are.equal(datetime:calculate_UTCdiff(t), datetime:calculate_UTCdiff(t))
    end)
  end)

  describe("Tests datetime:_get_pattern", function()
    it("caches and returns the same compiled pattern for a format", function()
      local fmt = "^%Y-%m-%d$"
      datetime._pattern_cache[fmt] = nil
      local p1 = datetime:_get_pattern(fmt)
      local p2 = datetime:_get_pattern(fmt)
      assert.is_not_nil(datetime._pattern_cache[fmt])
      assert.are.equal(p1, p2)
    end)

    it("compiles directives into a pattern that matches only valid input", function()
      local fmt = "^%Y-%m-%d$"
      datetime._pattern_cache[fmt] = nil
      local p = datetime:_get_pattern(fmt)
      assert.is_not_nil(p:tfind("2025-06-15"))
      assert.is_nil(p:tfind("not-a-date"))
    end)

    it("passes a directive it does not know through as a literal", function()
      -- %Z is not in _directives, so it stays in the regex as the two characters
      -- it is written with rather than becoming a capture group
      local fmt = "^%Z$"
      datetime._pattern_cache[fmt] = nil
      local p = datetime:_get_pattern(fmt)
      assert.is_not_nil(p:tfind("%Z"))
      assert.is_nil(p:tfind("Z"))
      datetime._pattern_cache[fmt] = nil
    end)

    it("compiles a format only once and hands the same pattern back", function()
      local fmt = "^dateTimeSpecUncachedFormat %Y$"
      datetime._pattern_cache[fmt] = nil
      local before = table.size(datetime._pattern_cache)
      local first = datetime:_get_pattern(fmt)
      assert.equals(before + 1, table.size(datetime._pattern_cache))
      assert.equals(first, datetime:_get_pattern(fmt))
      assert.equals(before + 1, table.size(datetime._pattern_cache))
      datetime._pattern_cache[fmt] = nil
    end)
  end)

end)
