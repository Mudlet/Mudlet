-----------------------------------------------------------------------------
-- Date / Time parsing functions.
-----------------------------------------------------------------------------
datetime = {
  _directives = {
    ["%b"] = "(?P<abbrev_month_name>jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)",
    ["%B"] = "(?P<month_name>january|february|march|april|may|june|july|august|september|october|november|december)",
    ["%d"] = "(?P<day_of_month>\\d{2})",
    ["%H"] = "(?P<hour_24>\\d{2})",
    ["%I"] = "(?P<hour_12>\\d{2})",
    ["%m"] = "(?P<month>\\d{2})",
    ["%M"] = "(?P<minute>\\d{2})",
    ["%p"] = "(?P<ampm>am|pm)",
    ["%S"] = "(?P<second>\\d{2})",
    ["%y"] = "(?P<year_half>\\d{2})",
    ["%Y"] = "(?P<year_full>\\d{4})"
  },
  _pattern_cache = {},
  _month_names = {
    ["january"] = 1,
    ["february"] = 2,
    ["march"] = 3,
    ["april"] = 4,
    ["may"] = 5,
    ["june"] = 6,
    ["july"] = 7,
    ["august"] = 8,
    ["september"] = 9,
    ["october"] = 10,
    ["november"] = 11,
    ["december"] = 12
  },
  _abbrev_month_names = {
    ["jan"] = 1,
    ["feb"] = 2,
    ["mar"] = 3,
    ["apr"] = 4,
    ["may"] = 5,
    ["jun"] = 6,
    ["jul"] = 7,
    ["aug"] = 8,
    ["sep"] = 9,
    ["oct"] = 10,
    ["nov"] = 11,
    ["dec"] = 12
  }
}

-- the timestamp is stored in UTC time, so work out the difference in seconds
-- from local to UTC time. Credit: https://github.com/stevedonovan/Penlight/blob/master/lua/pl/Date.lua#L85
function datetime:calculate_UTCdiff(ts)
  local date, time = os.date, os.time
  local utc = date('!*t', ts)
  local lcl = date('*t', ts)
  lcl.isdst = false
  return os.difftime(time(lcl), time(utc))
end


function datetime:_days_in_month(year, month)
  if month == 2 then
    local leap = (year % 4 == 0 and year % 100 ~= 0) or year % 400 == 0
    return leap and 29 or 28
  end
  return (month == 4 or month == 6 or month == 9 or month == 11) and 30 or 31
end


-- NOT LUADOC
-- The rex.match function does not return named patterns even if you use named capture
-- groups, but the r:tfind does -- but this only operates on compiled patterns. So,
-- we are caching the conversion of 'simple format' date patterns into a regex, and
-- then compiling them.
function datetime:_get_pattern(format)
  if not datetime._pattern_cache[format] then
    local fmt = rex.gsub(format, "(%[A-Za-z])",
    function(m)
      return datetime._directives[m] or m
    end
    )

    datetime._pattern_cache[format] = rex.new(fmt, rex.flags().CASELESS)
  end

  return datetime._pattern_cache[format]
end


--- Parses the specified source string, according to the format if given, to return a representation of
--- the date/time. The default format if not specified is: "^%Y-%m-%d %H:%M:%S$" <br/><br/>
---
--- If as_epoch is provided and true, the return value will be a Unix epoch -- the number
--- of seconds since 1970. This is a useful format for exchanging date/times with other systems. If as_epoch
--- is false, then a Lua time table will be returned. Details of the time tables are provided
--- in the http://www.lua.org/pil/22.1.html. <br/><br/>
---
--- A format without a date part (such as "^%H:%M:%S$") or with only part of one (such as "^%Y-%m$") returns
--- a time table with only the fields it parsed and no isdst, which os.time() cannot take. With as_epoch, a
--- format without a date part is placed on today's date, and a partial date takes the current year (and
--- month, when it has a day) and starts a missing month or day at 1, so "^%Y-%m$" gives the first of that
--- month. With as_epoch, a missing hour is 12 and a missing minute or second is 0, as in os.time(); a partial
--- date that does not exist (such as "02-29" outside a leap year, or the 31st in a 30-day month) returns nil. <br/><br/>
---
--- Supported Format Codes
---   </pre>
---   %b   Abbreviated Month Name
---   %B   Full Month Name
---   %d   Day of Month
---   %H   Hour (24-hour format)
---   %I   Hour (12-hour format, requires %p as well)
---   %p   AM or PM
---   %m   2-digit month (01-12)
---   %M   2-digit minutes (00-59)
---   %S   2-digit seconds (00-59)
---   %y   2-digit year (00-99), will automatically prepend 20 so 10 becomes 2010 and not 1910.
---   %Y   4-digit year.
---   </pre>
function datetime:parse(source, format, as_epoch)
  if not format then
    format = "^%Y-%m-%d %H:%M:%S$"
  end

  local dt = {}
  local fmt = datetime:_get_pattern(format)
  local m = { fmt:tfind(source) }

  if m and m[3] then
    m = m[3]

    if m.year_half then
      dt.year = tonumber("20" .. m.year_half)
    elseif m.year_full then
      dt.year = tonumber(m.year_full)
    end

    if m.month then
      dt.month = tonumber(m.month)
    elseif m.month_name then
      dt.month = datetime._month_names[m.month_name:lower()]
    elseif m.abbrev_month_name then
      dt.month = datetime._abbrev_month_names[m.abbrev_month_name:lower()]
    end

    dt.day = tonumber(m.day_of_month)

    if m.hour_12 then
      assert(m.ampm, "You must use %p (AM|PM) with %I (12-hour time)")
      -- 12-hour to 24-hour: 12 AM is 0, 12 PM is 12, so the 12 wraps to 0 before
      -- the PM offset is added. The regex is caseless, so compare caselessly too.
      dt.hour = tonumber(m.hour_12) % 12
      if m.ampm:upper() == "PM" then
        dt.hour = dt.hour + 12
      end
    else
      dt.hour = tonumber(m.hour_24)
    end

    dt.min = tonumber(m.minute)
    dt.sec = tonumber(m.second)

    if as_epoch and not (dt.year and dt.month and dt.day) then
      local today = os.date("*t")
      if not (dt.year or dt.month or dt.day) then
        dt.year, dt.month, dt.day = today.year, today.month, today.day
      end
      dt.year = dt.year or today.year
      dt.month = dt.month or (dt.day and today.month or 1)
      -- not today's day: it could be past the end of a parsed month
      dt.day = dt.day or 1
      -- os.time() would roll an impossible date (the 31st in a 30-day month, 29 February outside a leap year,
      -- month 13) into another day; check the date alone, as normalising the time can move the day too
      if dt.month < 1 or dt.month > 12 or dt.day < 1 or dt.day > datetime:_days_in_month(dt.year, dt.month) then
        return nil
      end
    end

    -- os.time() raises on a table without a full date
    if dt.year and dt.month and dt.day then
      dt.isdst = os.date("*t", os.time(dt))["isdst"]
    end

    if as_epoch then
      return os.time(dt)
    else
      return dt
    end
  else
    return nil
  end
end


--- Splits a count of seconds into zero-padded hours, minutes and seconds, dropping any fraction of a second.
--- Hours keep counting past 24. A count still negative once its fraction is dropped carries its minus sign
--- once, on the hours ("-00" under an hour), while the minutes and seconds stay unsigned, so read the sign
--- from the hours string, not tonumber(); a negative count under one second splits the same as zero.
--- @param seconds the count of seconds, as a number or a string holding one, smaller in size than 2^53
--- @param bool if true, cecho the result instead of returning it
--- @return hours, minutes and seconds as strings
function shms(seconds, bool)
  seconds = tonumber(seconds)
  -- past 2^53 a double stops holding whole seconds; NaN and infinities fail the comparison too
  assert(type(seconds) == "number" and math.abs(seconds) < 2^53, "Assertion failed for function 'shms' - Please supply a valid number.")

  local s = seconds
  local magnitude = math.floor(math.abs(s))
  local sign = (s < 0 and magnitude > 0) and "-" or ""
  local ss = string.format("%02d", magnitude % 60)
  local mm = string.format("%02d", math.floor(magnitude / 60) % 60)
  -- %d casts to a C long, which is 32 bits on Windows, so hours past 2^31 would wrap there
  local hh = sign .. string.format("%02.0f", math.floor(magnitude / (60 * 60)))

  if bool then
    cecho("<green>" .. s .. " <grey>seconds converts to: <green>" .. hh .. "<white>h,<green> " .. mm .. "<white>m <grey>and<green> " .. ss .. "<white>s.")
  else
    return hh, mm, ss
  end
end
