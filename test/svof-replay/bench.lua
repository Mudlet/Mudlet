-- Copies svof's config into this profile, installs svof, lets it start, then times the replay from the
-- BEGIN to the END marker. svof is the whole workload; the only triggers added here are the two markers.
-- Mode "socket" connects to run.sh's log server; mode "feed" plays the session in-process with
-- feedTelnet(), in a profile opened with --offline.
local CFG = _G.BENCH_CFG
local BUNDLE, RESULT = CFG.bundle, CFG.result
local manifest = dofile(BUNDLE .. "/manifest.lua")
-- run.sh loads this file from the checkout, the package from its own folder: gate.lua sits beside it
local gate = dofile(debug.getinfo(1, "S").source:match("^@(.*)/[^/]*$") .. "/gate.lua")
-- A profile reopened after a run would otherwise leave that run's report for whoever reads it next
os.remove(RESULT)
os.remove(RESULT .. ".dbg")
os.remove(RESULT .. ".buffer")

local function dbg(msg)
  local h = io.open(RESULT .. ".dbg", "a")
  if h then h:write(os.date("%H:%M:%S ") .. msg .. "\n"); h:close() end
end

-- Nothing trimmed during a pass, so the whole output can be gated
setConsoleBufferSize("main", 400000, 400000)
-- As a real Achaea profile: swallow the newline the game sends after a GA prompt
setConfig("fixUnnecessaryLinebreaks", true)

local function svofInstalled()
  for _, name in ipairs(getPackages() or {}) do
    if name == "svof" then return true end
  end
  return false
end

local function svoStatus()
  local s = _G.svo
  if type(s) ~= "table" then return "svo_loaded nil" end
  local defs = type(s.defencetemptriggers) == "table" and #s.defencetemptriggers or -1
  local hp = gmcp and gmcp.Char and gmcp.Char.Vitals and gmcp.Char.Vitals.hp
  return string.format("svo_loaded %s\nsvo_class %s\nsvo_deftriggers %d\nsvo_version %s\nsvo_prompts %s\nsvo_lastprompt %s\nsvo_paused %s\ngmcp_hp %s",
    tostring(s.systemloaded), tostring(s.me and s.me.class), defs, tostring(s.version), tostring(s.promptcount),
    tostring(s.lastpromptnumber), tostring(s.conf and s.conf.paused), tostring(hp))
end

local function report(text)
  local f = io.open(RESULT, "w")
  if f then
    f:write(text)
    f:close()
  end
  if CFG.mode == "feed" then
    echo("\n" .. text)
    if CFG.quit then tempTimer(1.5, function() closeMudlet() end) end
  else
    tempTimer(1.5, function() closeMudlet() end)
  end
end

local function fail(why)
  dbg("FAIL " .. why)
  report(string.format("label %s\nmode %s\nstop_reason setup\ngate FAIL: %s\n", CFG.label, CFG.mode, why))
end

-- A 16 ms timer chain: gaps above that are time the event loop could not run, painting included
local hbGaps, hbLast, hbOn = {}, nil, false
local function heartbeat()
  local t = getEpoch()
  if hbOn and hbLast then hbGaps[#hbGaps + 1] = t - hbLast end
  hbLast = t
  tempTimer(0.016, heartbeat)
end

local samples = {}
local running, finished = false, false
local stallTicks = 0
local beginLine = 0

local function stats(t)
  if #t == 0 then return nil end
  local s = {}
  for i, v in ipairs(t) do s[i] = v end
  table.sort(s)
  local sum = 0
  for _, v in ipairs(s) do sum = sum + v end
  local function p(q) return s[math.max(1, math.ceil(#s * q))] end
  return {n = #s, mean = sum / #s, p50 = p(0.5), p90 = p(0.9), p99 = p(0.99), max = s[#s]}
end

-- The replay's output; the socket mode also keeps it on disk for comparing builds
local function readBuffer(endLine)
  local out = {}
  -- getLines' end is exclusive; beginLine is the BEGIN marker, which "lines" does not count either
  for from = beginLine + 1, endLine, 5000 do
    for _, l in ipairs(getLines(from, math.min(from + 5000, endLine + 1))) do out[#out + 1] = l end
  end
  if CFG.mode == "socket" then
    local f = io.open(RESULT .. ".buffer", "w")
    if f then
      for _, l in ipairs(out) do f:write(l, "\n") end
      f:close()
    end
  end
  return out
end

local function finish(why)
  if finished then return end
  finished = true
  running = false
  hbOn = false

  local first = samples[1]
  local lastActive = #samples
  while lastActive > 1 and samples[lastActive].lines == samples[lastActive - 1].lines do
    lastActive = lastActive - 1
  end
  local last = samples[lastActive]

  local elapsed = last.t - first.t
  local lines = last.lines - first.lines
  local rate = elapsed > 0 and lines / elapsed or 0

  local buffer = readBuffer(last.lines)
  local corpus = io.open(BUNDLE .. "/" .. manifest.corpus, "rb")
  local checked = gate.check(buffer, gate.plainLines(corpus:read("*a")))
  corpus:close()

  local problems = {}
  local s = type(_G.svo) == "table" and _G.svo or {}
  if why ~= "end_marker" then problems[#problems + 1] = "stopped by " .. why end
  if s.systemloaded ~= true then problems[#problems + 1] = "svof did not load" end
  if not s.conf or s.conf.paused ~= false then problems[#problems + 1] = "svof is paused" end
  if (s.me and s.me.class) ~= manifest.class then problems[#problems + 1] = "svof's class is not " .. manifest.class end
  if s.promptcount ~= manifest.prompts then
    problems[#problems + 1] = string.format("svof saw %s of %d prompts", tostring(s.promptcount), manifest.prompts)
  end
  local ref = manifest.expect and manifest.expect[CFG.mode]
  if not ref then
    problems[#problems + 1] = "no reference counts for " .. CFG.mode .. " runs"
  else
    local got = {lines = lines, game_lines = checked.game_lines, added_lines = checked.added_lines}
    for _, k in ipairs({"lines", "game_lines", "added_lines"}) do
      local slack = manifest.slack[k]
      if math.abs(got[k] - ref[k]) > slack then
        problems[#problems + 1] = string.format("%s %d is more than %d from the reference %d", k, got[k], slack, ref[k])
      end
    end
  end

  local g = stats(hbGaps)
  local out = {}
  local function add(fmt, ...) out[#out + 1] = string.format(fmt, ...) end
  add("label %s\nmode %s\nbundle %d\ncorpus_md5 %s\nstop_reason %s\n", CFG.label, CFG.mode, manifest.bundle, manifest.md5, why)
  add("elapsed_s %.3f\nlines %d\nrate_lines_per_s %.1f\n", elapsed, lines, rate)
  add("cpu_s %.3f\n", (_G.benchCpu1 or 0) - (_G.benchCpu0 or 0))
  add("%s\n", svoStatus())
  if g then
    local blocked = 0
    for _, v in ipairs(hbGaps) do if v > 0.020 then blocked = blocked + (v - 0.016) end end
    add("hb_samples %d\nhb_mean_ms %.1f\nhb_p50_ms %.1f\nhb_p90_ms %.1f\nhb_p99_ms %.1f\nhb_max_ms %.1f\n",
      g.n, g.mean * 1000, g.p50 * 1000, g.p90 * 1000, g.p99 * 1000, g.max * 1000)
    add("hb_blocked_s %.3f\nhb_blocked_pct %.1f\n", blocked, elapsed > 0 and blocked / elapsed * 100 or 0)
  else
    add("hb_samples 0\n")
  end
  add("buffer_lines %d\ngame_lines %d\ngame_hash %s\nadded_lines %d\nadded_hash %s\nlock_lines %d\n",
    #buffer, checked.game_lines, checked.game_hash, checked.added_lines, checked.added_hash, checked.lock_lines)
  add(#problems == 0 and "gate pass\n" or "gate FAIL: %s\n", table.concat(problems, "; "))
  dbg(string.format("FINISHED %s elapsed=%.3f lines=%d rate=%.0f/s", why, elapsed, lines, rate))
  report(table.concat(out))
end

-- Fallback stop if the END marker never arrives: 10 s without a new line
local function sample()
  if finished then return end
  if running then
    local n = getLastLineNumber() or 0
    samples[#samples + 1] = {t = getEpoch(), lines = n}
    local prev = samples[#samples - 1]
    if prev and prev.lines == n then
      stallTicks = stallTicks + 1
      if stallTicks >= 40 and #samples > 8 then finish("stalled") end
    else
      stallTicks = 0
    end
    if #samples > 2400 then finish("sample_limit") end
  end
  tempTimer(0.25, sample)
end

local function feed()
  local f = io.open(BUNDLE .. "/" .. manifest.corpus, "rb")
  if not f then return fail("cannot read " .. manifest.corpus) end
  local data = f:read("*a")
  f:close()
  -- feedTelnet() reads <XX> as a byte and << or >> as one bracket, so the game's own brackets are doubled
  -- A read ending after a GA prompt leaves a blank line, so chunks are the 100,000 bytes a socket read takes
  local chunks = {"--- BEGIN MEASURING ---\r\n"}
  for pos = 1, #data, CFG.chunk do
    chunks[#chunks + 1] = (data:sub(pos, pos + CFG.chunk - 1):gsub("[<>]", "%0%0"))
  end
  chunks[#chunks + 1] = "--- END MEASURING ---\r\n"
  local n = 0
  local function nextChunk()
    if finished then return end
    n = n + 1
    local ok, err = feedTelnet(chunks[n])
    if not ok then return fail("feedTelnet: " .. tostring(err)) end
    -- One chunk per event-loop turn, as a socket's reads are taken, so timers and painting still run
    if n < #chunks then tempTimer(0, nextChunk) end
  end
  nextChunk()
end

local function go()
  dbg("status before replay: " .. svoStatus():gsub("\n", " "))
  if CFG.mode == "socket" then
    connectToServer("127.0.0.1", CFG.port)
  else
    feed()
  end
end

-- svof reads its config from the profile when it loads
local function copyConfig()
  local home = getMudletHomeDir()
  for _, file in ipairs(manifest.config) do
    local dir = home .. "/svo"
    lfs.mkdir(dir)
    for part in file:gmatch("([^/]+)/") do
      dir = dir .. "/" .. part
      lfs.mkdir(dir)
    end
    local from = io.open(BUNDLE .. "/svo/" .. file, "rb")
    local to = io.open(home .. "/svo/" .. file, "wb")
    if not from or not to then return false, "cannot copy svo/" .. file end
    local data = from:read("*a")
    from:close()
    local wrote, err = to:write(data)
    local closed, cerr = to:close()
    if not wrote or not closed then return false, "cannot write svo/" .. file .. ": " .. tostring(err or cerr) end
  end
  return true
end

local function setup()
  -- svof keeps state across sessions, so a second pass in one profile does different work
  if svofInstalled() then
    return fail("svof is already installed here, so this profile has run the benchmark; use a fresh one")
  end
  if CFG.mode == "feed" and not feedTelnet("") then
    return fail("feedTelnet refuses while connected; open the profile with --offline")
  end
  local ok, err = copyConfig()
  if not ok then return fail(err) end
  ok, err = installPackage(BUNDLE .. "/svof.mpackage")
  dbg("installPackage svof -> " .. tostring(ok) .. " " .. tostring(err))
  if not svofInstalled() then return fail("installing svof failed: " .. tostring(err)) end

  tempTimer(0.016, heartbeat)
  tempTimer(0.25, sample)
  tempExactMatchTrigger("--- BEGIN MEASURING ---", function()
    dbg("BEGIN")
    hbGaps, hbLast, hbOn = {}, getEpoch(), true
    beginLine = getLastLineNumber() or 0
    samples = {{t = getEpoch(), lines = beginLine}}
    running, stallTicks = true, 0
    _G.benchCpu0 = os.clock()
  end)
  tempTrigger("END MEASURING", function()
    dbg("END marker seen")
    _G.benchCpu1 = os.clock()
    samples[#samples + 1] = {t = getEpoch(), lines = getLastLineNumber() or 0}
    finish("end_marker")
  end)
  tempTimer(CFG.settle, go)
end

-- Not from inside the import of the package that loaded this
tempTimer(0, setup)
