-- Installs svof into this profile, lets it start, then connects to the log server and times the
-- replay from the BEGIN to the END marker. svof is the whole workload; the only triggers added here
-- are the two markers.
local CFG = _G.BENCH_CFG
local RESULT = CFG.result

local function dbg(msg)
  local h = io.open(RESULT .. ".dbg", "a")
  if h then h:write(os.date("%H:%M:%S ") .. msg .. "\n"); h:close() end
end

-- Nothing trimmed during a pass, so the whole output can be dumped for the gate
setConsoleBufferSize("main", 400000, 400000)
-- As a real Achaea profile: swallow the newline the game sends after a GA prompt
setConfig("fixUnnecessaryLinebreaks", true)

local have = false
for _, name in ipairs(getPackages() or {}) do if name == "svof" then have = true end end
if not have then
  local ok, err = installPackage(CFG.package)
  dbg("installPackage " .. CFG.package .. " -> " .. tostring(ok) .. " " .. tostring(err))
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

-- A 16 ms timer chain: gaps above that are time the event loop could not run, painting included
local hbGaps, hbLast, hbOn = {}, nil, false
local function heartbeat()
  local t = getEpoch()
  if hbOn and hbLast then hbGaps[#hbGaps + 1] = t - hbLast end
  hbLast = t
  tempTimer(0.016, heartbeat)
end
tempTimer(0.016, heartbeat)

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

local function dumpBuffer(endLine)
  local f = io.open(RESULT .. ".buffer", "w")
  if not f then return 0 end
  local n = 0
  for from = beginLine, endLine, 5000 do
    for _, l in ipairs(getLines(from, math.min(from + 4999, endLine))) do f:write(l, "\n"); n = n + 1 end
  end
  f:close()
  return n
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

  local dumped = dumpBuffer(last.lines)
  local g = stats(hbGaps)
  local f = io.open(RESULT, "w")
  f:write(string.format("label %s\nstop_reason %s\n", CFG.label, why))
  f:write(string.format("elapsed_s %.3f\nlines %d\nrate_lines_per_s %.1f\n", elapsed, lines, rate))
  f:write(string.format("cpu_s %.3f\n", (_G.benchCpu1 or 0) - (_G.benchCpu0 or 0)))
  f:write(string.format("dumped_lines %d\n", dumped))
  f:write(svoStatus() .. "\n")
  if g then
    f:write(string.format("hb_samples %d\nhb_mean_ms %.1f\nhb_p50_ms %.1f\nhb_p90_ms %.1f\nhb_p99_ms %.1f\nhb_max_ms %.1f\n",
      g.n, g.mean * 1000, g.p50 * 1000, g.p90 * 1000, g.p99 * 1000, g.max * 1000))
    local blocked = 0
    for _, v in ipairs(hbGaps) do if v > 0.020 then blocked = blocked + (v - 0.016) end end
    f:write(string.format("hb_blocked_s %.3f\nhb_blocked_pct %.1f\n", blocked, elapsed > 0 and blocked / elapsed * 100 or 0))
  else
    f:write("hb_samples 0\n")
  end
  f:close()
  dbg(string.format("FINISHED %s elapsed=%.3f lines=%d rate=%.0f/s dumped=%d", why, elapsed, lines, rate, dumped))
  tempTimer(1.5, function() quit() end)
end

-- Fallback stop if the END marker never arrives: 10 s without a new line
local function sample()
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

tempTimer(CFG.settle, function()
  dbg("status before connect: " .. svoStatus():gsub("\n", " "))
  connectToServer("127.0.0.1", CFG.port)
end)
