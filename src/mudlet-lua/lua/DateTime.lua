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

    dt.day = m.day_of_month

    if m.hour_12 then
      assert(m.ampm, "You must use %p (AM|PM) with %I (12-hour time)")
      if m.ampm == "PM" then
        dt.hour = 12 + tonumber(m.hour_12)
      else
        dt.hour = tonumber(m.hour_12)
      end
    else
      dt.hour = tonumber(m.hour_24)
    end

    dt.min = tonumber(m.minute)
    dt.sec = tonumber(m.second)
    dt.isdst = os.date("*t", os.time(dt))["isdst"]

    if as_epoch then
      return os.time(dt)
    else
      return dt
    end
  else
    return nil
  end
end


function shms(seconds, bool)
  seconds = tonumber(seconds)
  assert(type(seconds) == "number", "Assertion failed for function 'shms' - Please supply a valid number.")

  local s = seconds
  local ss = string.format("%02d", math.fmod(s, 60))
  local mm = string.format("%02d", math.fmod((s / 60 ), 60))
  local hh = string.format("%02d", (s / (60 * 60)))

  if bool then
    cecho("<green>" .. s .. " <grey>seconds converts to: <green>" .. hh .. "<white>h,<green> " .. mm .. "<white>m <grey>and<green> " .. ss .. "<white>s.")
  else
    return hh, mm, ss
  end
end
