-- Checks that a scrubbed package makes svof do what its raw log does: counts what svof wrote in run.sh runs
-- of each, by kind of line, and holds the scrubbed runs to the raw runs' own spread. See README.md.
-- usage: lua5.1 fidelity.lua <raw.mpackage> <scrubbed.mpackage> <raw result.txt>... -- <scrubbed result.txt>...
local here = arg[0]:match("^(.*)/[^/]*$") or "."
local gate = dofile(here .. "/gate.lua")

local function die(msg)
  io.stderr:write("fidelity: " .. msg .. "\n")
  os.exit(2)
end
local function q(s)
  return "'" .. s:gsub("'", "'\\''") .. "'"
end
local function unzipped(pkg, file)
  local p = io.popen("unzip -p " .. q(pkg) .. " " .. q(file) .. " 2>/dev/null")
  local data = p:read("*a")
  p:close()
  if #data == 0 then die("no " .. file .. " in " .. pkg) end
  return data
end

local rawPkg, scrPkg = arg[1], arg[2]
local results = {raw = {}, scrubbed = {}}
do
  local side = "raw"
  for i = 3, #arg do
    if arg[i] == "--" then side = "scrubbed" else table.insert(results[side], arg[i]) end
  end
end
if not scrPkg or #results.raw < 2 or #results.scrubbed < 1 then
  die("usage: lua5.1 fidelity.lua <raw.mpackage> <scrubbed.mpackage> <raw result.txt>... -- <scrubbed result.txt>...")
end

-- svof echoes game text: a stand-in used for one word only reads back to it, and any other filler as "~"
local rawCorpus, scrCorpus = unzipped(rawPkg, "corpus.txt"), unzipped(scrPkg, "corpus.txt")
local back, stable, inRaw, inScr, svofWords = {}, {}, {}, {}, {}
do
  local tmp = os.tmpname()
  local f = io.open(tmp, "wb")
  f:write(unzipped(scrPkg, "svof.mpackage"))
  f:close()
  local p = io.popen("unzip -p " .. q(tmp))
  for w in p:read("*a"):gmatch("%a+") do svofWords[w:lower()] = true end
  p:close()
  os.remove(tmp)
end
do
  local fwd, rev, ri = {}, {}, rawCorpus:gmatch("([^\n]*)\n")
  for s in scrCorpus:gmatch("([^\n]*)\n") do
    local r = ri()
    if not r or #r ~= #s then die(scrPkg .. "'s corpus is not a scrub of " .. rawPkg .. "'s") end
    for pos, w in s:gmatch("()(%a+)") do
      local rw = r:sub(pos, pos + #w - 1)
      inScr[w], inRaw[rw] = true, true
      if rev[w] == nil then rev[w] = rw elseif rev[w] ~= rw then rev[w] = false end
      if fwd[rw] == nil then fwd[rw] = w elseif fwd[rw] ~= w then fwd[rw] = false end
    end
  end
  for w, rw in pairs(rev) do
    if rw and fwd[rw] == w then
      back[w], stable[rw] = rw, true
    end
  end
end
-- Words of one or two letters are kept by svof's patterns in some lines and filler in others
local function kind(l, scrubbed)
  return (l:gsub("%d+", "0"):gsub("%a+", function(w)
    if scrubbed then
      w = back[w] or ((inScr[w] or (inRaw[w] and not stable[w])) and "~") or w
    elseif not stable[w] and (inRaw[w] or inScr[w]) then
      w = "~"
    end
    if not (inRaw[w] or inScr[w] or svofWords[w:lower()]) then w = "~" end
    return #w <= 2 and "~" or w
  end))
end

local function count(pkg, list)
  local manifest = loadstring(unzipped(pkg, "manifest.lua"))()
  local plain = gate.plainLines(pkg == rawPkg and rawCorpus or scrCorpus)
  local translate = pkg == scrPkg
  local runs = {}
  for _, path in ipairs(list) do
    local f = io.open(path) or die("cannot read " .. path)
    local res = {}
    for l in f:lines() do
      local k, v = l:match("^([%w_]+) (.*)$")
      if k then res[k] = v end
    end
    f:close()
    if res.corpus_md5 ~= manifest.md5 then die(path .. " is not a run of " .. pkg) end
    local lines = {}
    for l in io.lines(path .. ".buffer") do lines[#lines + 1] = l end
    local _, added, locks = gate.split(lines, plain)
    local kinds = {["all added lines"] = #added, ["all lock warnings"] = #locks}
    for _, l in ipairs(added) do
      local k = kind(l, translate)
      kinds[k] = (kinds[k] or 0) + 1
    end
    for _, l in ipairs(locks) do kinds[l] = (kinds[l] or 0) + 1 end
    runs[#runs + 1] = {kinds = kinds, elapsed = tonumber(res.elapsed_s)}
  end
  return runs
end

local function median(t)
  local s = {}
  for i, v in ipairs(t) do s[i] = v end
  table.sort(s)
  local n = #s
  return n % 2 == 1 and s[(n + 1) / 2] or (s[n / 2] + s[n / 2 + 1]) / 2
end

local raw, scr = count(rawPkg, results.raw), count(scrPkg, results.scrubbed)
local all = {}
for _, runs in ipairs({raw, scr}) do
  for _, r in ipairs(runs) do
    for k in pairs(r.kinds) do all[k] = true end
  end
end
local rows, failed = {}, 0
for k in pairs(all) do
  local rv, sv = {}, {}
  for i, r in ipairs(raw) do rv[i] = r.kinds[k] or 0 end
  for i, r in ipairs(scr) do sv[i] = r.kinds[k] or 0 end
  local spread = math.max(unpack(rv)) - math.min(unpack(rv))
  local delta = median(sv) - median(rv)
  local ok = math.abs(delta) <= spread
  if not ok then failed = failed + 1 end
  rows[#rows + 1] = {k = k, raw = median(rv), spread = spread, delta = delta, ok = ok, total = k:match("^all ")}
end
table.sort(rows, function(a, b)
  if (a.total ~= nil) ~= (b.total ~= nil) then return a.total ~= nil end
  if a.ok ~= b.ok then return not a.ok end
  return math.abs(a.delta) > math.abs(b.delta) or (math.abs(a.delta) == math.abs(b.delta) and a.k < b.k)
end)
print(string.format("%d raw runs, %d scrubbed runs; a kind of line passes when the scrubbed median is within the raw runs' spread",
  #raw, #scr))
print("   raw  spread  scrubbed-raw  kind")
for _, r in ipairs(rows) do
  if r.total or not r.ok then
    print(string.format("%6g  %6d  %12g  %s%s", r.raw, r.spread, r.delta, r.k:sub(1, 100), r.ok and "" or "  FAIL"))
  end
end
local re, se = {}, {}
for i, r in ipairs(raw) do re[i] = r.elapsed end
for i, r in ipairs(scr) do se[i] = r.elapsed end
local line = string.format("elapsed_s median raw %.3f, scrubbed %.3f, ratio %.3f", median(re), median(se), median(se) / median(re))
if #re == #se then
  local faster = 0
  for i = 1, #re do if se[i] < re[i] then faster = faster + 1 end end
  line = line .. string.format("; scrubbed faster in %d of %d pairs", faster, #re)
end
print(line)
print(failed == 0 and "fidelity pass" or string.format("fidelity FAIL: %d kinds of line outside the raw spread", failed))
os.exit(failed == 0 and 0 or 1)
