-- The generic mapper finds a room's name by reading back over the lines that
-- arrived since the last command, so it has to see each line once; and as it
-- sees every line, it has to do so cheaply.
describe("Tests that the generic mapper keeps the lines it reads a room from", function()
  local function upvalue(fn, name)
    local index = 1
    while true do
      local upvalueName, value = debug.getupvalue(fn, index)
      if not upvalueName then
        return nil
      end
      if upvalueName == name then
        return value
      end
      index = index + 1
    end
  end

  local grabLine = type(map) == "table" and type(map.eventHandler) == "function"
    and upvalue(map.eventHandler, "grab_line")
  if type(grabLine) ~= "function" then
    it("needs the generic mapper installed", function()
      pending("the generic mapper is not installed in this profile")
    end)
    return
  end

  -- the mapper replaces its table of lines whenever it clears it, so the
  -- table is looked up afresh each time it is read
  local function timesSeen(text)
    local count = 0
    for _, seen in ipairs(upvalue(grabLine, "lines")) do
      if seen == text then
        count = count + 1
      end
    end
    return count
  end

  it("reads each line that arrives once", function()
    local marker = ("genericMapperLine-%d-%d"):format(os.time(), math.random(100000))
    local lines = upvalue(grabLine, "lines")
    local keptLines = #lines
    finally(function()
      if upvalue(grabLine, "lines") == lines then
        for index = #lines, keptLines + 1, -1 do
          lines[index] = nil
        end
      end
    end)
    -- the leading newline closes any line an earlier spec left open
    feedTriggers("\n" .. marker .. "\n")
    assert.are.equal(1, timesSeen(marker))
    feedTriggers(marker .. "\n")
    assert.are.equal(2, timesSeen(marker))
  end)

  -- Lua 5.1 builds an arg table on every call of a vararg function that reads arg
  it("handles an event without building a table for it", function()
    local event = "genericMapperSpecUnhandledEvent"
    map.eventHandler(event, "first", "second")
    finally(function()
      collectgarbage("restart")
    end)
    collectgarbage("stop")
    local before = collectgarbage("count")
    for _ = 1, 10000 do
      map.eventHandler(event, "first", "second")
    end
    local grownKb = collectgarbage("count") - before
    assert.is_true(grownKb < 64, ("grew %.0f KB over 10000 calls"):format(grownKb))
  end)
end)
