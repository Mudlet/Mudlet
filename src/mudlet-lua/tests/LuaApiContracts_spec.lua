-- Contract tests for the parts of the Lua API where the refusal *is* the
-- behaviour: which message comes back, whether it arrives as nil plus a reason
-- or as a raised error, and what the profile looks like afterwards. Scripts
-- branch on those answers, so changing one is a breaking change even though
-- nothing about the successful path moved.

local function contains(haystack, needle)
  return type(haystack) == "string" and haystack:find(needle, 1, true) ~= nil
end

-- Matching a message substring rather than merely "did it error?" is what
-- proves the call reached its own argument validation: a function that had
-- been renamed away would raise "attempt to call a nil value" and satisfy a
-- bare has_error just as well.
local function assertArgError(fn, needle)
  local ok, err = pcall(fn)
  assert.is_false(ok, "the call was accepted instead of raising")
  assert.is_true(contains(err, needle), tostring(err))
end

local function assertRefused(needle, ok, err)
  assert.is_nil(ok, "the call was accepted instead of refused")
  assert.is_true(contains(err, needle), tostring(err))
end

-- Mudlet's own colour numbering predates the ANSI one and interleaves it: 1 is
-- light black, 2 is black, 3 is light red, 4 is red, and so on up to 16, white.
-- isAnsiFgColor, isAnsiBgColor and tempColorTrigger all speak it, and the saved
-- profile format stores it, so renumbering silently changes what every colour
-- trigger a player already has matches.
local sgrForIndex = {
  fg = {90, 30, 91, 31, 92, 32, 93, 33, 94, 34, 95, 35, 96, 36, 97, 37},
  bg = {100, 40, 101, 41, 102, 42, 103, 43, 104, 44, 105, 45, 106, 46, 107, 47},
}

describe("Tests the legacy ANSI colour numbering", function()

  local mark

  local function feed(data)
    local ok, msg = feedTelnet(data)
    assert.is_true(ok, "start the suite with --offline, see the tests README - feedTelnet said: " .. tostring(msg))
  end

  -- the buffer line the marker landed on, looked for from where this test's own
  -- output starts so that an earlier test's identical text cannot answer
  local function lineNumberOf(marker)
    local last = getLastLineNumber("main")
    local lines = getLines("main", mark, last + 1)
    for i = #lines, 1, -1 do
      if lines[i]:find(marker, 1, true) then
        return mark + i - 1
      end
    end
    return nil
  end

  -- isAnsiFgColor and isAnsiBgColor read the first character of the selection
  local function selectMarker(marker)
    local line = lineNumberOf(marker)
    assert.is_truthy(line, "no line carrying '" .. marker .. "' reached the buffer")
    assert.is_true(moveCursor("main", 0, line))
    assert.is_true(selectString(marker, 1) >= 0, "'" .. marker .. "' was not selectable on the line it landed on")
  end

  -- Index 0 is the profile's default colour, and the defaults are white on
  -- black, so 0 legitimately answers true alongside 16 for a foreground and
  -- alongside 2 for a background. It is left out of the sweep for that reason.
  local function assertOnlyColour(reader, index, marker)
    assert.is_true(reader(index), marker .. " did not answer to colour " .. index)
    for other = 1, 16 do
      if other ~= index then
        assert.is_false(reader(other), marker .. " also answered to colour " .. other)
      end
    end
  end

  local function paintAndSelect(kind, index)
    local marker = ("AnsiNum%s%d"):format(kind, index)
    mark = getLastLineNumber("main")
    feed(("\27[0m\27[%dm%s\r\n"):format(sgrForIndex[kind][index], marker))
    selectMarker(marker)
    return marker
  end

  after_each(function()
    deselect()
    -- selecting a marker parks the user cursor partway up the buffer, and every
    -- spec after this one reads the buffer from wherever it was left
    moveCursorEnd()
  end)

  it("gives isAnsiFgColor a distinct number for each foreground colour", function()
    for index = 1, 16 do
      local marker = paintAndSelect("fg", index)
      assertOnlyColour(isAnsiFgColor, index, marker)
      deselect()
    end
  end)

  it("gives isAnsiBgColor a distinct number for each background colour", function()
    for index = 1, 16 do
      local marker = paintAndSelect("bg", index)
      assertOnlyColour(isAnsiBgColor, index, marker)
      deselect()
    end
  end)

  -- the out of range refusals are already pinned, message and all, by
  -- UI_spec.lua's "isAnsiFgColor/isAnsiBgColor error handling"

  it("raises rather than guessing when the colour number is not a number", function()
    paintAndSelect("fg", 4)
    assertArgError(function() return isAnsiFgColor("red") end, "bad argument #1 type")
    assertArgError(function() return isAnsiBgColor() end, "got no value")
  end)
end)

describe("Tests the functionality of tempColorTrigger", function()

  local created

  before_each(function()
    created = {}
  end)

  after_each(function()
    for _, id in ipairs(created) do
      killTrigger(id)
    end
  end)

  local function track(id)
    created[#created + 1] = id
    return id
  end

  it("matches the colour its number names and no other", function()
    local fired = {}
    for index = 1, 16 do
      track(tempColorTrigger(index, -1, function() fired[index] = true end))
    end

    for index = 1, 16 do
      for key in pairs(fired) do
        fired[key] = nil
      end
      feedTriggers(("\27[0m\27[%dmColourTriggerNumbering\n"):format(sgrForIndex.fg[index]))

      local hits = {}
      for key in pairs(fired) do
        hits[#hits + 1] = key
      end
      table.sort(hits)
      assert.are.same({index}, hits, ("SGR %d should have fired only the colour %d trigger"):format(sgrForIndex.fg[index], index))
    end
  end)

  it("refuses to ignore both colours at once, since that would match everything", function()
    assertRefused("only one of foreground and background colors can be -1", tempColorTrigger(-1, -1, "noop"))
  end)

  it("refuses an expiry count that would expire the trigger before it ever fires", function()
    assertRefused("must be greater than zero, got 0", tempColorTrigger(4, -1, "noop", 0))
    assertRefused("must be greater than zero, got -3", tempColorTrigger(4, -1, "noop", -3))
  end)

  it("raises on arguments it cannot make sense of, naming the one at fault", function()
    assertArgError(function() return tempColorTrigger("red", -1, "noop") end, "bad argument #1 type")
    assertArgError(function() return tempColorTrigger(4, "red", "noop") end, "bad argument #2 type")
    assertArgError(function() return tempColorTrigger(4, -1, {}) end, "bad argument #3 type")
    assertArgError(function() return tempColorTrigger(4, -1, "noop", "soon") end, "bad argument #4 value")
  end)

  -- Trigger IDs come off a counter that only a created trigger advances, so a
  -- gap in them is the visible trace of a refusal that built something first
  -- and then walked away from it
  it("builds nothing when it refuses", function()
    local first = track(tempColorTrigger(4, -1, "noop"))

    tempColorTrigger(-1, -1, "noop")
    tempColorTrigger(4, -1, "noop", 0)
    pcall(function() return tempColorTrigger(4, -1, {}) end)
    pcall(function() return tempColorTrigger("red", -1, "noop") end)

    assert.are.equal(first + 1, track(tempColorTrigger(4, -1, "noop")), "a refused tempColorTrigger consumed a trigger ID")
  end)
end)

describe("Tests the functionality of tempAnsiColorTrigger", function()

  local created

  before_each(function()
    created = {}
  end)

  after_each(function()
    for _, id in ipairs(created) do
      killTrigger(id)
    end
  end)

  -- -1 ignores a colour and -2 matches whatever the profile's default is; the
  -- messages have to keep saying which is which, because they are the only
  -- place a script author is told
  it("names both sentinel colours when refusing one outside the range", function()
    assertRefused("only -1 (ignore foreground color)", tempAnsiColorTrigger(999, 0, "noop"))
    assertRefused("only -1 (ignore background color)", tempAnsiColorTrigger(0, 999, "noop"))
    assertRefused("invalid ANSI color number -3", tempAnsiColorTrigger(-3, 0, "noop"))
  end)

  it("tells the two ways of ignoring everything apart", function()
    -- with a background given but also ignored
    assertRefused("you cannot ignore both foreground and background color", tempAnsiColorTrigger(-1, -1, "noop"))
    -- with the background left out entirely, which is a different mistake
    assertRefused("(omitted)", tempAnsiColorTrigger(-1, "noop"))
    assertRefused("if the background color is omitted", tempAnsiColorTrigger(-1))
  end)

  it("refuses an expiry count that would expire the trigger before it ever fires", function()
    assertRefused("must be nil or greater than zero, got 0", tempAnsiColorTrigger(0, 1, "noop", 0))
  end)

  it("raises on arguments it cannot make sense of, naming the one at fault", function()
    assertArgError(function() return tempAnsiColorTrigger("red", 0, "noop") end, "bad argument #1 type")
    -- the background is optional, so it is only read as one when the argument
    -- count says it must be - hence the expiry count here
    assertArgError(function() return tempAnsiColorTrigger(0, "red", "noop", 1) end, "bad argument #2 type")
    assertArgError(function() return tempAnsiColorTrigger(0, 1, {}) end, "bad argument #3 type")
    assertArgError(function() return tempAnsiColorTrigger(0, 1, "noop", "soon") end, "bad argument #4 value")
  end)

  it("builds nothing when it refuses", function()
    local first = tempAnsiColorTrigger(0, 1, "noop")
    created[#created + 1] = first

    tempAnsiColorTrigger(999, 0, "noop")
    tempAnsiColorTrigger(-1, -1, "noop")
    tempAnsiColorTrigger(0, 1, "noop", 0)
    pcall(function() return tempAnsiColorTrigger(0, 1, {}) end)
    pcall(function() return tempAnsiColorTrigger("red", 0, "noop") end)

    local second = tempAnsiColorTrigger(0, 1, "noop")
    created[#created + 1] = second
    assert.are.equal(first + 1, second, "a refused tempAnsiColorTrigger consumed a trigger ID")
  end)

  it("accepts a background-only match, which is the one -1 that is allowed", function()
    local fired = false
    local id = tempAnsiColorTrigger(-1, 1, function() fired = true end)
    assert.is_number(id)
    created[#created + 1] = id

    feedTriggers("\27[0m\27[41mAnsiColourBackgroundOnly\n")
    assert.is_true(fired, "a background-only trigger did not match a line painted in that background")
  end)
end)

describe("Tests what feedTriggers will and will not carry", function()

  local original

  before_each(function()
    original = getServerEncoding()
  end)

  after_each(function()
    setServerEncoding(original)
  end)

  local function textFrom(mark)
    return table.concat(getLines("main", mark, getLastLineNumber("main") + 1), "")
  end

  it("refuses text the game's encoding cannot carry instead of mangling it", function()
    assert.is_true(setServerEncoding("ASCII"))
    local mark = getLastLineNumber("main")

    local ok, err = feedTriggers("FeedEncRejected \195\169\n")
    assert.is_nil(ok, "text outside the game encoding should be refused")
    assert.is_true(contains(err, "cannot be conveyed in the current game server encoding of 'ASCII'"), tostring(err))
    assert.is_false(contains(textFrom(mark), "FeedEncRejected"), "the refused text was put on screen regardless")
  end)

  it("transcodes into the game's encoding when it can", function()
    assert.is_true(setServerEncoding("ISO 8859-1"))
    local mark = getLastLineNumber("main")

    assert.is_true(feedTriggers("FeedEncLatin \195\169\n"))
    assert.is_true(contains(textFrom(mark), "FeedEncLatin \195\169"), "the accented character did not survive the round trip")
  end)

  -- The second argument false is the older form: the caller has already encoded
  -- the bytes themselves, so Mudlet must pass them through untouched rather
  -- than reading them as UTF-8 and rejecting or double-encoding them
  it("takes bytes already in the game's encoding when told they are not UTF-8", function()
    assert.is_true(setServerEncoding("ISO 8859-1"))
    local mark = getLastLineNumber("main")

    assert.is_true(feedTriggers("FeedEncRaw \233\n", false))
    assert.is_true(contains(textFrom(mark), "FeedEncRaw \195\169"), "the pre-encoded byte did not arrive as the character it stands for")
  end)

  it("raises on arguments it cannot make sense of", function()
    assertArgError(function() return feedTriggers({}) end, "bad argument #1 type")
    assertArgError(function() return feedTriggers("FeedEncNever\n", "yes") end, "bad argument #2 type")
  end)
end)

describe("Tests announce and showNotification", function()

  local processingKinds = {"importantall", "importantmostrecent", "all", "mostrecent", "currentthenmostrecent"}

  -- The message is the only place a script author is told which processing
  -- styles a screen reader will take, so dropping the list from it removes the
  -- documentation along with the names
  it("names every processing style announce accepts when refusing one", function()
    local ok, err = pcall(announce, "spec announcement", "sideways")
    assert.is_false(ok, "an unknown processing style should be refused, not passed on")
    for _, kind in ipairs(processingKinds) do
      assert.is_true(contains(err, kind), kind .. " was left out of the refusal: " .. tostring(err))
    end
  end)

  it("accepts every processing style it lists", function()
    for _, kind in ipairs(processingKinds) do
      assert.is_true(pcall(announce, "spec announcement", kind), kind .. " is offered in the refusal but refused when used")
    end
    assert.is_true(pcall(announce, "spec announcement"), "the processing style is meant to be optional")
  end)

  it("raises when there is nothing to announce", function()
    assertArgError(function() return announce() end, "text to announce as string expected")
  end)

  -- showNotification's refusals are pinned in Miscallaneous_spec.lua, among the
  -- other functions whose effect needs a desktop; only the accepted argument
  -- counts are left to cover
  it("takes a title on its own, a message with it, and an expiry with both", function()
    assert.is_true(showNotification("Mudlet spec notification"))
    assert.is_true(showNotification("Mudlet spec notification", "with a body"))
    assert.is_true(showNotification("Mudlet spec notification", "with a body", 1))
  end)
end)

describe("Tests the functionality of alert", function()

  -- zero is a duration, so the boundary is where the refusal starts rather
  -- than "anything falsy is rejected"
  it("takes a duration of zero but refuses one below it", function()
    assert.is_true(pcall(alert, 0), "zero seconds is a duration, not a mistake")
    assertArgError(function() return alert(-0.001) end, "is optional but if given must be zero or greater")
  end)

  it("takes no duration at all", function()
    assert.is_true(pcall(alert))
  end)

  it("raises when the duration is not a number", function()
    assertArgError(function() return alert("soon") end, "alert duration in seconds as number expected")
  end)
end)
