-- Builds SvofBench.mpackage, everything the svof replay benchmark needs in one file: svof, its saved
-- config, a prompted corpus with players' names and their words replaced, and the scripts that run it
-- (bench.lua, gate.lua, and standalone mode's loader). See README.md.
-- usage: lua5.1 make-package.lua --corpus <prompted-log> --svof <svof.mpackage> --config <svo-folder>
--            --out <SvofBench.mpackage> [--names <file>] [--bundle-version <n>] [--no-scrub]
local here = arg[0]:match("^(.*)/[^/]*$") or "."

local opts = {}
do
  local i = 1
  while arg[i] do
    local k = arg[i]:match("^%-%-(.+)$")
    if not k then opts = nil break end
    if k == "no-scrub" then
      opts[k] = true
      i = i + 1
    else
      opts[k] = arg[i + 1]
      i = i + 2
    end
  end
end
if not opts or not (opts.corpus and opts.svof and opts.config and opts.out) then
  io.stderr:write("usage: lua5.1 make-package.lua --corpus <prompted-log> --svof <svof.mpackage> --config <svo-folder>\n"
    .. "           --out <SvofBench.mpackage> [--names <file>] [--bundle-version <n>] [--no-scrub]\n")
  os.exit(2)
end

local function die(msg)
  io.stderr:write("make-package: " .. msg .. "\n")
  os.exit(1)
end

local function q(s)
  return "'" .. s:gsub("'", "'\\''") .. "'"
end

local function run(cmd)
  local ok = os.execute(cmd)
  if ok ~= 0 and ok ~= true then die("failed: " .. cmd) end
end

local function slurp(path)
  local f = io.open(path, "rb") or die("cannot read " .. path)
  local data = f:read("*a")
  f:close()
  return data
end

local function spit(path, data)
  local f = io.open(path, "wb") or die("cannot write " .. path)
  f:write(data)
  f:close()
end

local IAC = "\255"

-- A line as alternating text and telnet sequences, so the text can be rewritten around the protocol
local function segments(line)
  local segs, pos = {}, 1
  while pos <= #line do
    local s = line:find(IAC, pos, true)
    if not s then
      segs[#segs + 1] = {text = line:sub(pos)}
      break
    end
    if s > pos then segs[#segs + 1] = {text = line:sub(pos, s - 1)} end
    local cmd, e = line:byte(s + 1), nil
    if cmd == 250 then
      e = select(2, line:find("\255\240", s + 2, true)) or #line
    elseif cmd and cmd >= 251 and cmd <= 254 then
      e = s + 2
    else
      e = s + 1
    end
    segs[#segs + 1] = {proto = line:sub(s, e)}
    pos = e + 1
  end
  return segs
end

local corpus = slurp(opts.corpus)
local lines = {}
do
  local pos = 1
  while true do
    local nl = corpus:find("\n", pos, true)
    if not nl then
      lines[#lines + 1] = corpus:sub(pos)
      break
    end
    lines[#lines + 1] = corpus:sub(pos, nl - 1)
    pos = nl + 1
  end
end

local texts, prompt = {}, {}
for i, l in ipairs(lines) do
  local t = {}
  for _, s in ipairs(segments(l)) do
    if s.text then t[#t + 1] = s.text elseif s.proto == "\255\249" then prompt[i] = true end
  end
  texts[i] = table.concat(t)
end

local _, prompts = corpus:gsub("\255\249", "")
local _, newlines = corpus:gsub("\n", "")
local class = corpus:match('\255\250\201Char%.Status [^\255]-"class": "([^"]+)"')
if prompts == 0 then die("the corpus has no IAC GA prompts; build it with promptify.lua") end
if not class then die("the corpus has no GMCP Char.Status with a class; build it with promptify.lua") end

-- svof's trigger patterns. A word in one is never renamed, and each line the scrub changes must match the
-- same ones as before, except one-character patterns under another trigger: those only see a prompt's flags.
local svof = {words = {}, exact = {}, begins = {}, beginsBy = {}, subs = {}, literals = {}, regex = {}}
do
  local p = io.popen("unzip -p " .. q(opts.svof) .. " '*.xml'")
  local xml = p:read("*a")
  p:close()
  if #xml == 0 then die("no XML in " .. opts.svof) end
  local function unxml(s)
    return (s:gsub("&lt;", "<"):gsub("&gt;", ">"):gsub("&quot;", '"'):gsub("&apos;", "'"):gsub("&amp;", "&"))
  end
  local function keep(lit)
    lit = lit:match("^%s*(.-)%s*$")
    if #lit >= 6 and lit:find("%a") then svof.literals[#svof.literals + 1] = lit end
  end
  -- Runs of plain text in a regex: any of them in a line means the regex may match it
  local function regexLiterals(re)
    local run, i, n = {}, 1, #re
    local function flush()
      keep(table.concat(run))
      run = {}
    end
    while i <= n do
      local c = re:sub(i, i)
      if c == "\\" then
        local e = re:sub(i + 1, i + 1)
        if e:match("%w") then flush() else run[#run + 1] = e end
        i = i + 2
      elseif c == "[" then
        flush()
        local j = i + 1
        if re:sub(j, j) == "^" then j = j + 1 end
        if re:sub(j, j) == "]" then j = j + 1 end
        while j <= n and re:sub(j, j) ~= "]" do
          if re:sub(j, j) == "\\" then j = j + 1 end
          j = j + 1
        end
        i = j + 1
      elseif c == "(" then
        flush()
        i = i + 1
        if re:sub(i, i) == "?" then i = (re:find("[:=!>)]", i) or n) + 1 end
      elseif c == "?" or c == "*" or c == "{" then
        run[#run] = nil
        flush()
        i = c == "{" and (re:find("}", i, true) or n) + 1 or i + 1
      elseif c:match("[%^%$%.|)+]") then
        flush()
        i = i + 1
      else
        run[#run + 1] = c
        i = i + 1
      end
    end
    flush()
  end
  -- One entry per open trigger: whether it has patterns, so its children only run once it fires
  local open, pos = {}, 1
  while true do
    local s, e, close, tag = xml:find("<(/?)(%a+)[^>]*>", pos)
    if not s then break end
    pos = e + 1
    if (tag == "Trigger" or tag == "TriggerGroup") and xml:sub(e - 1, e) ~= "/>" then
      if close == "/" then open[#open] = nil else open[#open + 1] = false end
    elseif tag == "regexCodeList" and close == "" and xml:sub(e - 1, e) ~= "/>" then
      local listEnd = xml:find("</regexCodeList>", pos, true)
      local _, typesEnd, types = xml:find("^%s*<regexCodePropertyList>(.-)</regexCodePropertyList>", listEnd + 16)
      if not typesEnd then die("unexpected trigger XML in " .. opts.svof) end
      local kinds = {}
      for k in types:gmatch("<integer>(%-?%d+)</integer>") do kinds[#kinds + 1] = tonumber(k) end
      local top = true
      for k = 1, #open - 1 do top = top and not open[k] end
      local n = 0
      for pat in xml:sub(pos, listEnd - 1):gmatch("<string>(.-)</string>") do
        n = n + 1
        pat = unxml(pat)
        open[#open] = true
        for w in pat:gmatch("%a+") do svof.words[w:lower()] = true end
        local kind, checked = kinds[n], top or #pat >= 2
        if kind == 3 and checked then
          svof.exact[pat] = true
        elseif kind == 2 and checked then
          svof.begins[#svof.begins + 1] = pat
          local b = pat:byte(1)
          svof.beginsBy[b] = svof.beginsBy[b] or {}
          table.insert(svof.beginsBy[b], pat)
        elseif kind == 0 then
          keep(pat)
          if checked then svof.subs[#svof.subs + 1] = pat end
        elseif kind == 1 then
          regexLiterals(pat)
          if top then svof.regex[#svof.regex + 1] = pat end
        end
      end
      pos = typesEnd + 1
    end
  end
end

-- Which of svof's checked patterns match a line
local function signature(t)
  local hits = {}
  if svof.exact[t] then hits[1] = "=" end
  for _, p in ipairs(svof.beginsBy[t:byte(1) or 0] or {}) do
    if t:sub(1, #p) == p then hits[#hits + 1] = "^" .. p end
  end
  for _, p in ipairs(svof.subs) do
    if t:find(p, 1, true) then hits[#hits + 1] = "~" .. p end
  end
  return table.concat(hits, "\0")
end

-- Columns of a line that a checked pattern matches on, so filler leaves them as they are
local function pinsOf(t)
  local pins = {}
  if svof.exact[t] then
    for c = 1, #t do pins[c] = true end
    return pins
  end
  for _, p in ipairs(svof.beginsBy[t:byte(1) or 0] or {}) do
    if t:sub(1, #p) == p then
      for c = 1, #p do pins[c] = true end
    end
  end
  for _, p in ipairs(svof.subs) do
    local s, e = t:find(p, 1, true)
    while s do
      for c = s, e do pins[c] = true end
      s, e = t:find(p, s + 1, true)
    end
  end
  return pins
end

-- Whether svof may match line i on text inside columns a..b
local function svofNeeds(i, a, b)
  local t = texts[i]
  if svof.exact[t] then return true end
  for _, p in ipairs(svof.begins) do
    if #p >= a and t:sub(1, #p) == p then return true end
  end
  for _, lit in ipairs(svof.literals) do
    local s = t:find(lit, math.max(1, a - #lit + 1), true)
    if s and s <= b then return true end
  end
  return false
end

-- Deterministic: the same inputs build the same package
local function seedOf(s)
  local h = 5381
  for k = 1, #s do h = (h * 33 + s:byte(k)) % 2147483647 end
  return h == 0 and 1 or h
end
local function rng(seed)
  local x = seed
  return function(n)
    x = x * 16807 % 2147483647
    return x % n + 1
  end
end
local VOWELS, CONSONANTS, DIGITS = "aeiou", "bcdfghjklmnprstvwz", "0123456789"
local function reshape(s, roll, pins, from)
  return (s:gsub("()(%w)", function(col, c)
    if pins and pins[from + col - 1] then return c end
    local pool = c:match("%d") and DIGITS or c:lower():match("[aeiouy]") and VOWELS or CONSONANTS
    local k = roll(#pool)
    local r = pool:sub(k, k)
    return c:match("%u") and r:upper() or r
  end))
end

local stats = {names = 0, speech = 0, speechLines = 0, unclosed = 0, emotes = 0, emoteLines = 0, kept = 0, commands = 0}
local spans = {}
local function scrubSpan(i, a, b)
  if b < a then return end
  spans[i] = spans[i] or {}
  table.insert(spans[i], {a, b})
end

local SPEECH = {}
for w in ([[say says tell tells ask asks exclaim exclaims yell yells shout shouts whisper whispers sing sings
  reply replies answer answers cry cries mutter mutters murmur murmurs declare declares intone intones
  recite recites chant chants call calls croon croons growl growls hiss hisses snarl snarls remark remarks
  state states add adds continue continues note notes bellow bellows decree decrees]]):gmatch("%a+") do
  SPEECH[w] = true
end

local TYPED = {"%f[%w][pcgo]t %S", "%f[%w]clt%d* %S", "%f[%w]tell %a+ %S", "%f[%w]say %S", "%f[%w]reply %S"}

-- First column inside the quotes when line i is someone speaking
local function speechOpens(i)
  local t = texts[i]
  local s = t:find('[,:] "')
  if not s or s > 150 then return nil end
  for w in t:sub(1, s):gmatch("%a+") do
    if SPEECH[w] then return s + 3 end
  end
end

local names, people = {}, {}
if not opts["no-scrub"] then
  local cap, low = {}, {}
  for _, t in ipairs(texts) do
    for w in t:gmatch("%a+") do
      if w:match("^%u%l+$") then cap[w] = (cap[w] or 0) + 1 elseif w:match("^%l+$") then low[w] = (low[w] or 0) + 1 end
    end
  end

  -- Speakers, those told something and those acting at the start of a line; a word mostly seen lowercase is
  -- not a name. Mid-sentence, a capital never seen lowercase is a name, or a place or item renamed with them.
  local candidates, midSentence = {}, {}
  for i, t in ipairs(texts) do
    for p, w in t:gmatch("()%f[%a](%u%l%l+)%f[%A]") do
      if t:sub(math.max(1, p - 3), p - 1):match("%a,? $") then midSentence[w] = true end
    end
    local body = t:gsub("^%([^)]+%): ", "")
    local w, rest = body:match("^(%u%l+)[ ,'](.*)")
    if w and (speechOpens(i) or (rest:match("^(%l+)") or ""):match("s$")) then candidates[w] = true end
    local told = t:match("^You tell (%u%l+), ")
    if told then candidates[told] = true end
  end
  local status = corpus:match('\255\250\201Char%.Status [^\255]-"name": "(%u%l+)"')
  if status then candidates[status] = true end
  if opts.names then
    for w in io.lines(opts.names) do
      w = w:match("^%s*(%u%l+)%s*$")
      if w then candidates[w] = "listed" end
    end
  end
  for w in pairs(midSentence) do
    if not low[w:lower()] and not candidates[w] then candidates[w] = true end
  end
  -- Any other capital never seen lowercase may be a name too: it is renamed, but not taken for a speaker
  local sorted = {}
  for w, c in pairs(cap) do
    local l, why = low[w:lower()] or 0, candidates[w]
    if not svof.words[w:lower()] and w ~= class then
      if why == "listed" or (why and #w >= 3 and not w:match("ing$") and l * 8 <= c) then people[w] = true end
      if people[w] or (#w >= 3 and l == 0) then sorted[#sorted + 1] = w end
    end
  end
  table.sort(sorted)

  local taken = {}
  for _, t in ipairs(texts) do
    for w in t:gmatch("%a+") do taken[w:lower()] = true end
  end
  local function sameMatches(a, b)
    return signature(a) == signature(b) and signature(a:lower()) == signature(b:lower())
      and signature(a:upper()) == signature(b:upper())
  end
  for _, w in ipairs(sorted) do
    local roll, pins, fake, tries = rng(seedOf(w)), pinsOf(w), nil, 0
    repeat
      fake = reshape(w, roll, pins, 1)
      tries = tries + 1
      if tries > 1000 then die("no free name of the shape of " .. w) end
    until not taken[fake:lower()] and not svof.words[fake:lower()] and sameMatches(w, fake)
    taken[fake:lower()] = true
    names[w] = fake
    if (low[w:lower()] or 0) * 8 <= (cap[w] or 0) then
      names[w:lower()] = fake:lower()
      names[w:upper()] = fake:upper()
    end
  end
  stats.names, stats.people = #sorted, 0
  for _ in pairs(people) do stats.people = stats.people + 1 end

  local function startsWithName(t)
    local w = t:match("^(%u%l+)[ ,']")
    return w and people[w] and w
  end
  local function template(t)
    return (t:gsub("%a+", function(w) if names[w] then return "N" end end):gsub("%d+", "#"))
  end
  local seen = {}
  for i, t in ipairs(texts) do
    if startsWithName(t) then
      local k = template(t)
      seen[k] = (seen[k] or 0) + 1
    end
  end

  local i = 1
  while i <= #texts do
    local t = texts[i]
    local open = not prompt[i] and speechOpens(i)
    if open then
      -- The quote may wrap onto later lines, with a prompt inserted between them
      local close = t:find('"', open, true)
      local last, stop = i, nil
      if close then
        scrubSpan(i, open, close - 1)
      else
        local j = i + 1
        while j <= #texts and j <= i + 30 and not stop do
          if texts[j] == "" then
            stop = j - 1
          elseif not prompt[j] then
            local c = texts[j]:find('"', 1, true)
            if c then stop = j end
          end
          j = j + 1
        end
        if stop then
          scrubSpan(i, open, #t)
          for k = i + 1, stop do
            if not prompt[k] then
              local c = texts[k]:find('"', 1, true)
              scrubSpan(k, 1, (c or #texts[k] + 1) - 1)
              stats.speechLines = stats.speechLines + 1
            end
          end
          last = stop
        else
          scrubSpan(i, open, #t)
          stats.unclosed = stats.unclosed + 1
        end
      end
      stats.speech = stats.speech + 1
      stats.speechLines = stats.speechLines + 1
      i = last + 1
    else
      local who = not prompt[i] and startsWithName(t)
      -- Game messages repeat with other names; a line seen once is someone's own words
      if who and seen[template(t)] == 1 then
        if svofNeeds(i, #who + 2, #t) then
          stats.kept = stats.kept + 1
        else
          scrubSpan(i, #who + 2, #t)
          stats.emotes = stats.emotes + 1
          stats.emoteLines = stats.emoteLines + 1
          local j = i
          while not texts[j]:match('[%.!%?"\')]%s*$') and j < i + 3 and texts[j + 1] and texts[j + 1] ~= ""
            and not prompt[j + 1] and not startsWithName(texts[j + 1]) and not speechOpens(j + 1)
            and not svofNeeds(j + 1, 1, #texts[j + 1]) do
            j = j + 1
            scrubSpan(j, 1, #texts[j])
            stats.emoteLines = stats.emoteLines + 1
          end
          i = j
        end
      elseif prompt[i] then
        -- A channel or tell typed at the prompt
        local s
        for _, p in ipairs(TYPED) do s = s or t:find(p) end
        if s then
          scrubSpan(i, t:find(" ", s, true) + 1, #t)
          stats.commands = stats.commands + 1
        end
      end
      i = i + 1
    end
  end
end

local function rename(s)
  return (s:gsub("%a+", names))
end

-- With lrexlib, svof's top-level regexes are checked too; without it only their literal text is kept
local hasRex, rex = pcall(require, "rex_pcre")
local regexes = {}
if hasRex then
  for _, p in ipairs(svof.regex) do
    local ok, r = pcall(rex.new, p)
    if ok then regexes[#regexes + 1] = r end
  end
end
local function fullSignature(t)
  local s = signature(t)
  if hasRex then
    local hits = {}
    for k, r in ipairs(regexes) do
      if r:exec(t) then hits[#hits + 1] = k end
    end
    s = s .. "\0" .. table.concat(hits, ",")
  end
  return s
end

-- Filler is rerolled until the line matches the same svof patterns as before
local function scrubbedText(i)
  local t = texts[i]
  if not spans[i] then
    local r = rename(t)
    if r ~= t and fullSignature(r) ~= fullSignature(t) then
      die("line " .. i .. ": renaming changes which svof patterns match it: " .. t)
    end
    return r
  end
  local want, pins = fullSignature(t), pinsOf(t)
  for attempt = 1, 200 do
    local s = t
    for _, span in ipairs(spans[i]) do
      local a, b = span[1], span[2]
      s = s:sub(1, a - 1) .. reshape(s:sub(a, b), rng(seedOf(i .. ":" .. a .. ":" .. attempt)), pins, a) .. s:sub(b + 1)
    end
    s = rename(s)
    if fullSignature(s) == want then
      stats.rerolls = stats.rerolls + attempt - 1
      return s
    end
  end
  die("line " .. i .. ": no filler matches the same svof patterns: " .. t)
end

stats.rerolls = 0
local out = {}
for i, l in ipairs(lines) do
  if not spans[i] and not next(names) then
    out[i] = l
  else
    local t, pos, parts = scrubbedText(i), 1, {}
    for _, s in ipairs(segments(l)) do
      if s.text then
        parts[#parts + 1] = t:sub(pos, pos + #s.text - 1)
        pos = pos + #s.text
      elseif s.proto:sub(2, 2) == "\250" then
        -- Only the character's name: svof reads the other GMCP values
        parts[#parts + 1] = s.proto:gsub('("%l*name": ")([^"]*)', function(k, v) return k .. rename(v) end)
      else
        parts[#parts + 1] = s.proto
      end
    end
    out[i] = table.concat(parts)
    if #out[i] ~= #l then die("line " .. i .. " changed length") end
  end
end
local scrubbed = table.concat(out, "\n")
if #scrubbed ~= #corpus or select(2, scrubbed:gsub("\255\249", "")) ~= prompts then die("the scrub changed the corpus's shape") end

local stage = opts.out .. ".stage"
run("rm -rf " .. q(stage) .. " && mkdir -p " .. q(stage .. "/svo"))
spit(stage .. "/corpus.txt", scrubbed)
local md5 = io.popen("md5sum " .. q(stage .. "/corpus.txt")):read("*l"):match("^(%x+)")
run("cp " .. q(opts.svof) .. " " .. q(stage .. "/svof.mpackage"))
run("cp " .. q(here .. "/bench.lua") .. " " .. q(here .. "/gate.lua") .. " " .. q(stage))

-- What svof reads at start; downloads/ only tracks its update checks
local config = {}
for _, dir in ipairs({"config", "defup+keepup", "namedb", "pipes", "prios", "rift+inv"}) do
  local p = io.popen("cd " .. q(opts.config) .. " && find " .. q(dir) .. " -type f 2>/dev/null | LC_ALL=C sort")
  for f in p:lines() do config[#config + 1] = f end
  p:close()
end
if #config == 0 then die("no svof config in " .. opts.config) end
for _, f in ipairs(config) do
  local data = slurp(opts.config .. "/" .. f)
  if f == "config/my_class" then
    data = data:gsub("(%[%[class%]%] *%] *= *)%[%[[^%]]*%]%]", "%1[[" .. class .. "]]")
  elseif f == "config/conf" then
    -- A saved config is usually paused; a player unpauses it at the start of a session
    data = data:gsub("(%[%[paused%]%] *%] *= *)true", "%1false")
  end
  run("mkdir -p " .. q(stage .. "/svo/" .. f:match("^(.*)/")))
  spit(stage .. "/svo/" .. f, data)
end

local version = tonumber(opts["bundle-version"] or "1") or die("--bundle-version takes a number")
local cfgList = {}
for _, f in ipairs(config) do cfgList[#cfgList + 1] = string.format("%q", f) end
spit(stage .. "/manifest.lua", string.format([[return {
  bundle = %d,
  class = %q,
  prompts = %d,
  lines = %d,
  bytes = %d,
  corpus = "corpus.txt",
  md5 = %q,
  scrubbed = %s,
  config = {%s},
}
]], version, class, prompts, newlines, #scrubbed, md5, tostring(not opts["no-scrub"]), table.concat(cfgList, ", ")))

spit(stage .. "/config.lua", string.format([[mpackage = [==[SvofBench]==]
author = [==[Mudlet]==]
title = [==[svof replay benchmark]==]
description = [==[Times svof curing a recorded Achaea session. Install into a fresh profile opened with --offline.]==]
version = [==[%d]==]
]], version))

spit(stage .. "/SvofBench.xml", [[<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
	<ScriptPackage>
		<Script isActive="yes" isFolder="no">
			<name>SvofBench</name>
			<packageName></packageName>
			<script>local dir = getMudletHomeDir() .. "/SvofBench"
_G.BENCH_CFG = {mode = "feed", bundle = dir, result = dir .. "/result.txt", label = "standalone", settle = 4.0, chunk = 100000, quit = os.getenv("SVOF_BENCH_QUIT") == "1"}
dofile(dir .. "/bench.lua")</script>
			<eventHandlerList />
		</Script>
	</ScriptPackage>
</MudletPackage>
]])

local files = {"config.lua", "SvofBench.xml", "manifest.lua", "bench.lua", "gate.lua", "svof.mpackage", "corpus.txt"}
for _, f in ipairs(config) do files[#files + 1] = "svo/" .. f end
local quoted = {}
for k, f in ipairs(files) do quoted[k] = q(f) end
local outPath = opts.out:match("^/") and opts.out or (os.getenv("PWD") .. "/" .. opts.out)
run("rm -f " .. q(outPath))
-- Fixed timestamps, so an unchanged input rebuilds to an identical file
run("cd " .. q(stage) .. " && export TZ=UTC && find . -exec touch -h -d '2026-01-01 00:00:00' {} + && zip -q -X -9 -D "
  .. q(outPath) .. " " .. table.concat(quoted, " "))
run("rm -rf " .. q(stage))

print(string.format("%s: class %s, %d lines, %d prompts, corpus md5 %s", opts.out, class, newlines, prompts, md5))
if not opts["no-scrub"] then
  print(string.format("scrubbed: %d names (%d people), %d speeches (%d lines, %d never closed), %d emotes (%d lines, %d kept for svof), %d typed",
    stats.names, stats.people, stats.speech, stats.speechLines, stats.unclosed, stats.emotes, stats.emoteLines, stats.kept, stats.commands))
  print(string.format("every changed line matches the same svof patterns (%d filler rerolls; regexes %s)", stats.rerolls,
    hasRex and "checked" or "not checked, lrexlib-pcre is missing"))
end
