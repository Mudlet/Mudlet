-- verify-mudlet stages this as the only spec of a replay run: it plays a recorded game session into a
-- fresh offline profile and writes down what the profile made of it, for the caller to diff between builds.
local cfg = dofile(assert(os.getenv("MUDLET_VERIFY_REPLAY_CONFIG"), "MUDLET_VERIFY_REPLAY_CONFIG is not set"))

local function writeLines(name, lines)
  local file = assert(io.open(cfg.out .. "/" .. name, "wb"))
  for _, line in ipairs(lines) do
    file:write(line, "\n")
  end
  file:close()
end

local function show(value)
  local kind = type(value)
  if kind == "string" then
    return (string.format("%q", value):gsub("\\\n", "\\n"))
  elseif kind == "number" or kind == "boolean" or kind == "nil" then
    return tostring(value)
  end
  -- a table's or function's address differs from run to run
  return "<" .. kind .. ">"
end

local function copyTree(from, to)
  lfs.mkdir(to)
  for entry in lfs.dir(from) do
    if entry ~= "." and entry ~= ".." then
      local source, target = from .. "/" .. entry, to .. "/" .. entry
      if lfs.attributes(source, "mode") == "directory" then
        copyTree(source, target)
      else
        local input = assert(io.open(source, "rb"))
        local data = input:read("*a")
        input:close()
        local output = assert(io.open(target, "wb"))
        output:write(data)
        output:close()
      end
    end
  end
end

local function pump(ms)
  while ms > 0 do
    local step = math.min(ms, 30000)
    assert(pumpEvents(step))
    ms = ms - step
  end
end

describe("verify-mudlet replay", function()
  it("plays the session and records what the profile did", function()
    assert.is_true(feedTelnet(""), "feedTelnet refuses while connected: open the profile with --offline")
    -- nothing may scroll out of the buffer before it is written down
    setConsoleBufferSize("main", 2000000, 1000)

    if cfg.home then
      copyTree(cfg.home, getMudletHomeDir())
    end
    if cfg.setup then
      dofile(cfg.setup)
    end
    for _, package in ipairs(cfg.packages) do
      local ok, err = installPackage(package)
      assert(ok, "installPackage(" .. package .. "): " .. tostring(err))
    end
    pump(cfg.settle)

    local events, sends = {}, {}
    local first = math.max(getLineCount("main") - 1, 0)
    local handler = registerAnonymousEventHandler("*", function(event, ...)
      local parts = {event}
      for i = 1, select("#", ...) do
        parts[#parts + 1] = show((select(i, ...)))
      end
      events[#events + 1] = table.concat(parts, " ")
      if event == "sysDataSendRequest" then
        sends[#sends + 1] = string.format("%d\t%s", getLineCount("main") - first, tostring((...)))
      end
    end)

    local file = assert(io.open(cfg.log, "rb"))
    local data = file:read("*a")
    file:close()
    for pos = 1, #data, cfg.chunk do
      -- feedTelnet() reads <XX> as a byte and << or >> as one bracket, so the game's own brackets are doubled
      local ok, err = feedTelnet((data:sub(pos, pos + cfg.chunk - 1):gsub("[<>]", "%0%0")))
      assert(ok, "feedTelnet: " .. tostring(err))
      if cfg.pump > 0 then
        pump(cfg.pump)
      end
    end
    pump(cfg.after)
    killAnonymousEventHandler(handler)

    local last = getLineCount("main")
    local console = {}
    if cfg.plain then
      console = getLines("main", first, last)
    else
      for line = first, last - 1 do
        moveCursor("main", 0, line)
        console[#console + 1] = copy2decho("main")
      end
      moveCursorEnd("main")
    end

    writeLines("console.txt", console)
    writeLines("sends.txt", sends)
    writeLines("events.txt", events)
  end)
end)
