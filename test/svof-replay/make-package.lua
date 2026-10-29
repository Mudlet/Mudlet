-- Builds SvofBench.mpackage, everything the svof replay benchmark needs in one file: svof, its saved
-- config, a prompted corpus with players' names and their words replaced, and the scripts that run it
-- (bench.lua, gate.lua, and standalone mode's loader). See README.md.
local USAGE = [[
usage: lua5.1 make-package.lua --corpus <prompted-log> --svof <svof.mpackage> --config <svo-folder>
           --out <SvofBench.mpackage> [--namedb <Database_namedb.db>] [--names <file>]
           [--expect <result.txt>]... [--bundle-version <n>] [--no-scrub]
]]
local here = arg[0]:match("^(.*)/[^/]*$") or "."

local function usage(msg)
  io.stderr:write("make-package: " .. msg .. "\n" .. USAGE)
  os.exit(2)
end

local VALUED = {corpus = true, svof = true, config = true, out = true, namedb = true, names = true,
  expect = true, ["bundle-version"] = true}
local opts, expects = {}, {}
do
  local i = 1
  while arg[i] do
    local k = arg[i]:match("^%-%-(.+)$")
    if k == "no-scrub" then
      opts[k] = true
      i = i + 1
    elseif k and VALUED[k] then
      local v = arg[i + 1]
      if not v or v:match("^%-%-") then usage("--" .. k .. " needs a value") end
      if k == "expect" then
        expects[#expects + 1] = v
      elseif opts[k] then
        usage("--" .. k .. " is given twice")
      else
        opts[k] = v
      end
      i = i + 2
    else
      usage("unknown argument " .. arg[i])
    end
  end
end
if not (opts.corpus and opts.svof and opts.config and opts.out) then
  usage("--corpus, --svof, --config and --out are required")
end
local scrub = not opts["no-scrub"]

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
  local ok, err = f:write(data)
  local closed, cerr = f:close()
  if not ok or not closed then die("cannot write " .. path .. ": " .. tostring(err or cerr)) end
end

local function exists(path)
  local f = io.open(path, "rb")
  if f then f:close() end
  return f ~= nil
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
if corpus:sub(-1) ~= "\n" then die("the corpus ends mid-line, so it was cut short: " .. opts.corpus) end
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

-- svof's trigger patterns. A word in one is only renamed if it is a listed name, and each line the scrub
-- changes must match the same ones as before, except one-character patterns under another trigger: those
-- only see a prompt's flags.
local svof = {words = {}, exact = {}, beginsBy = {}, subs = {}, regex = {}, text = {}}
do
  local p = io.popen("unzip -p " .. q(opts.svof) .. " '*.xml'")
  local xml = p:read("*a")
  p:close()
  if #xml == 0 then die("no XML in " .. opts.svof) end
  local function unxml(s)
    return (s:gsub("&lt;", "<"):gsub("&gt;", ">"):gsub("&quot;", '"'):gsub("&apos;", "'"):gsub("&amp;", "&"))
  end
  -- Runs of plain text in a regex
  local function regexRuns(re)
    local runs, run, i, n = {}, {}, 1, #re
    local function flush()
      if #run > 0 then runs[#runs + 1] = table.concat(run) end
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
    return runs
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
        if kind == 0 or kind == 2 or kind == 3 then svof.text[#svof.text + 1] = pat end
        if kind == 3 and checked then
          svof.exact[pat] = true
        elseif kind == 2 and checked then
          local b = pat:byte(1)
          svof.beginsBy[b] = svof.beginsBy[b] or {}
          table.insert(svof.beginsBy[b], pat)
        elseif kind == 0 and checked then
          svof.subs[#svof.subs + 1] = pat
        elseif kind == 1 then
          local runs = regexRuns(pat)
          for _, r in ipairs(runs) do svof.text[#svof.text + 1] = r end
          svof.regex[#svof.regex + 1] = {pat = pat, runs = runs}
        end
      end
      pos = typesEnd + 1
    end
  end
end

-- Every regex, nested ones included, is checked
local regexes = {}
if scrub then
  local ok, rex = pcall(require, "rex_pcre")
  if not ok then die("scrubbing needs lrexlib-pcre (rex_pcre) to check svof's regexes") end
  for _, r in ipairs(svof.regex) do
    local compiled, re = pcall(rex.new, r.pat)
    if not compiled then die("PCRE cannot compile svof's regex " .. r.pat) end
    regexes[#regexes + 1] = {re = re, runs = r.runs}
  end
end

-- Which of svof's checked patterns match a line, and with pins, the columns their literal text sits on
local function analyse(t, pins)
  local hits = {}
  local function pin(s, e)
    for c = s, e do pins[c] = true end
  end
  local function pinAll(lit)
    local s, e = t:find(lit, 1, true)
    while s do
      pin(s, e)
      s, e = t:find(lit, s + 1, true)
    end
  end
  if svof.exact[t] then
    hits[#hits + 1] = "="
    if pins then pin(1, #t) end
  end
  for _, p in ipairs(svof.beginsBy[t:byte(1) or 0] or {}) do
    if t:sub(1, #p) == p then
      hits[#hits + 1] = "^" .. p
      if pins then pin(1, #p) end
    end
  end
  for _, p in ipairs(svof.subs) do
    if t:find(p, 1, true) then
      hits[#hits + 1] = "~" .. p
      if pins then pinAll(p) end
    end
  end
  for k, r in ipairs(regexes) do
    if r.re:exec(t) then
      hits[#hits + 1] = "/" .. k
      if pins then
        for _, run in ipairs(r.runs) do pinAll(run) end
      end
    end
  end
  return table.concat(hits, "\0")
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

-- Without the log's timestamp, or the ">" a line after a prompt can start with
local function bare(t)
  return (t:gsub("^>", ""):gsub("^%d%d:%d%d:%d%d%.%d+ ", ""))
end

local fakes, listed, people = {}, {}, {}
local function rename(s)
  return (s:gsub("%a+", function(w)
    local f = fakes[w:lower()]
    if not f then return nil end
    return (f:gsub("()(%a)", function(c, ch) return w:sub(c, c):match("%u") and ch:upper() or nil end))
  end))
end

local stats = {listed = {}, renamed = 0, quotes = 0, quoteLines = 0, retro = 0, unclosed = 0, typed = 0,
  emotes = 0, emoteLines = 0, kept = 0, rerolls = 0}
local hard, soft = {}, {}
local function span(list, i, a, b)
  if b < a then return end
  list[i] = list[i] or {}
  table.insert(list[i], {a, b})
end

if scrub then
  -- Everything inside double quotes, whoever says it. A quote opens after a space or at the start of a line
  -- and closes before a space, punctuation or the end of one; any other quote flips the state. Prompts
  -- inserted into wrapped speech leave it open, and a blank line closes it.
  local RETRO = 10
  local inside, startCol, outLine, outCol, blockStart = false, nil, nil, nil, 1
  local function opens()
    inside = true
    stats.quotes = stats.quotes + 1
  end
  for k, t in ipairs(texts) do
    if prompt[k] then
      -- left as is: prompts carry svof's vitals
    elseif t:match("^%s*$") then
      if inside then stats.unclosed = stats.unclosed + 1 end
      inside, outLine, blockStart = false, nil, k + 1
    else
      startCol = 1
      local pos = 1
      while true do
        local c = t:find('"', pos, true)
        if not c then break end
        local prev, nxt = t:sub(c - 1, c - 1), t:sub(c + 1, c + 1)
        if c == 1 then prev = "" end
        local opener = (prev == "" or prev:match("[%s%(%[{]")) and nxt ~= "" and not nxt:match("%s")
        local closer = prev ~= "" and not prev:match("%s") and (nxt == "" or nxt:match("[%s%.,;:!%?%)%]}'%-]"))
        if opener == closer then opener = not inside end
        if opener then
          if not inside then
            opens()
            startCol = c + 1
          end
        elseif inside then
          span(hard, k, startCol, c - 1)
          inside = false
          outLine, outCol = k, c + 1
        else
          -- A close with no open: the opening was missed, so everything since the last close was speech
          local fromLine, fromCol = outLine or blockStart, outLine and outCol or 1
          if k - fromLine > RETRO then fromLine, fromCol = k - RETRO, 1 end
          for m = fromLine, k do
            if not prompt[m] then
              span(hard, m, m == fromLine and fromCol or 1, m == k and c - 1 or #texts[m])
            end
          end
          stats.retro = stats.retro + 1
          outLine, outCol = k, c + 1
        end
        pos = c + 1
      end
      if inside then
        span(hard, k, startCol, #t)
        stats.quoteLines = stats.quoteLines + 1
      end
    end
  end
  if inside then stats.unclosed = stats.unclosed + 1 end

  -- How often each word is seen capitalised, and in lower case outside prompts (with or without GA), the
  -- commands typed after them and speech, where names are often typed in lower case
  local cap, low, taken = {}, {}, {}
  for i, t in ipairs(texts) do
    local promptish = prompt[i] or t:find("%d+h, %d+m") or bare(t):find("^H:%d+[| ]")
    for col, w in t:gmatch("()(%a+)") do
      local l = w:lower()
      taken[l] = true
      if w:match("^%u%l+$") then
        cap[l] = (cap[l] or 0) + 1
      elseif not promptish and w:match("^%l+$") then
        local quoted = false
        for _, sp in ipairs(hard[i] or {}) do quoted = quoted or (col >= sp[1] and col <= sp[2]) end
        if not quoted then low[l] = (low[l] or 0) + 1 end
      end
    end
  end

  -- Real names: svof's name database, who-lists in the corpus, --names and GMCP. Each is renamed in any case.
  local function list(w, source)
    local l = w:lower()
    if l == class:lower() then return end
    if not listed[l] then
      listed[l] = source
      stats.listed[source] = (stats.listed[source] or 0) + 1
    end
  end
  if not opts.namedb then
    local guess = opts.config:gsub("/+$", "") .. "/../Database_namedb.db"
    if exists(guess) then opts.namedb = guess end
  end
  if opts.namedb then
    local p = io.popen("sqlite3 -readonly -batch -noheader " .. q(opts.namedb) .. " 'select name from people' 2>&1")
    local out = p:read("*a")
    local ok = p:close()
    if (ok ~= 0 and ok ~= true) or out:find("Error") then die("cannot read names from " .. opts.namedb .. ": " .. out) end
    for w in out:gmatch("[^\n]+") do
      if not w:match("^%a+$") then die("unexpected name in " .. opts.namedb .. ": " .. w) end
      list(w, "namedb")
    end
  end
  if opts.names then
    for w in io.lines(opts.names) do
      w = w:match("^%s*(.-)%s*$")
      if w ~= "" then
        if not w:match("^%a+$") then die(opts.names .. " lists something other than a name: " .. w) end
        list(w, "names")
      end
    end
  end
  for payload in corpus:gmatch("\255\250\201(.-)\255\240") do
    for w in payload:gmatch('"name": "(%a+)"') do list(w, "gmcp") end
  end

  -- QW lists ("Abysal, Aedesynne, ..., and Zriek.", then "(81 total)"), svof's QW by city, and CWHO rows
  local function listNames(t)
    t = bare(t):gsub("^%s*%a+%s*%(%d+%):", "")
    local names = {}
    for item in (t .. ","):gmatch("([^,]*),") do
      item = item:gsub("^%s+", ""):gsub("[%s%.]+$", ""):gsub("^and ", "")
      if item ~= "" then
        if not item:match("^%u%l+$") then return nil end
        names[#names + 1] = item
      end
    end
    return #names > 0 and names
  end
  local function nextText(j)
    while j <= #texts and prompt[j] do j = j + 1 end
    return j
  end
  local whoLine = {}
  local i = 1
  while i <= #texts do
    local t = bare(texts[i])
    if not prompt[i] and t:match("^%(svof%): qw") then
      local j = nextText(i + 1)
      while j <= #texts and listNames(texts[j]) do
        for _, w in ipairs(listNames(texts[j])) do list(w, "who") end
        whoLine[j] = true
        j = nextText(j + 1)
      end
      i = j
    elseif not prompt[i] and t:match("^Citizen%s+Rank%s+CT%s+Class%s*$") then
      local j = nextText(i + 1)
      if bare(texts[j] or ""):match("^%-+%s") then j = nextText(j + 1) end
      while j <= #texts do
        local col = bare(texts[j]):match("^(%u.-)%s%s+%d+%s+O[nf]f?%s")
        if not col then break end
        -- Titles share the column with names: a name is a capitalised word never seen in lower case outside speech
        for w in col:gmatch("%a+") do
          if w:match("^%u%l+$") and not low[w:lower()] and not svof.words[w:lower()] then list(w, "who") end
        end
        whoLine[j] = true
        j = nextText(j + 1)
      end
      i = j
    elseif not prompt[i] and listNames(texts[i]) then
      local run, j = {}, i
      while j <= #texts and listNames(texts[j]) do
        run[#run + 1] = j
        j = nextText(j + 1)
      end
      if j <= #texts and texts[j]:find("%(%d+ total%)") then
        for _, k in ipairs(run) do
          for _, w in ipairs(listNames(texts[k])) do list(w, "who") end
          whoLine[k] = true
        end
      end
      i = j
    else
      i = i + 1
    end
  end

  -- Others found by how they appear: speakers, people told something and those acting at the start of a
  -- line, and any capitalised word never seen in lower case in game text. A word mostly seen in lower
  -- case is not a name, and a word svof's patterns use is left as it is.
  local SPEECH = {}
  for w in ([[say says tell tells ask asks exclaim exclaims yell yells shout shouts whisper whispers sing sings
    reply replies answer answers cry cries mutter mutters murmur murmurs declare declares intone intones
    recite recites chant chants call calls croon croons growl growls hiss hisses snarl snarls remark remarks
    state states add adds continue continues note notes bellow bellows decree decrees]]):gmatch("%a+") do
    SPEECH[w] = true
  end
  local found = {}
  for k, t in ipairs(texts) do
    if not prompt[k] then
      local b = bare(t):gsub("^%([^)]+%): ", "")
      local w, rest = b:match("^(%u%l+)[ ,'](.*)")
      if w then
        local speaks = false
        local s = b:find('[,:] "')
        if s and s <= 150 then
          for v in b:sub(1, s):gmatch("%a+") do speaks = speaks or SPEECH[v] end
        end
        if speaks or (rest:match("^(%l+)") or ""):match("s$") then found[w] = true end
      end
      w = b:match("^You tell (%u%l+), ") or b:match("^(%u%l+) tells you, ")
      if w then found[w] = true end
    end
  end
  local function common(l)
    return svof.words[l] or l == class:lower() or #l < 3
  end
  for w in pairs(found) do
    local l = w:lower()
    if not common(l) and not w:match("ing$") and (low[l] or 0) * 8 <= (cap[l] or 0) then people[l] = true end
  end
  for l in pairs(listed) do people[l] = true end
  local sorted = {}
  for l in pairs(people) do sorted[#sorted + 1] = l end
  for l in pairs(cap) do
    if not people[l] and not low[l] and not common(l) then sorted[#sorted + 1] = l end
  end
  table.sort(sorted)

  for l in pairs(listed) do taken[l] = true end
  for l in pairs(svof.words) do taken[l] = true end
  local function sameMatches(a, b)
    local function title(s) return s:sub(1, 1):upper() .. s:sub(2) end
    return analyse(a) == analyse(b) and analyse(title(a)) == analyse(title(b)) and analyse(a:upper()) == analyse(b:upper())
  end
  for _, l in ipairs(sorted) do
    local roll, fake, tries, pins = rng(seedOf(l)), nil, 0, {}
    analyse(l, pins)
    analyse(l:sub(1, 1):upper() .. l:sub(2), pins)
    analyse(l:upper(), pins)
    repeat
      fake = reshape(l, roll, pins, 1)
      tries = tries + 1
      if tries > 1000 then die("no free name of the shape of a " .. #l .. "-letter name") end
    until not taken[fake] and sameMatches(l, fake)
    taken[fake] = true
    fakes[l] = fake
  end
  stats.renamed = #sorted

  -- What the player typed, in any case: channels, tells, says and emotes, after a prompt or echoed on a
  -- line of its own. A long one wraps onto lines before the game's "You ..." reply.
  local TYPED = {"say", "tell", "whisper", "reply", "shout", "yell", "emote", "[pcgo]t", "clt%d*", "newbie",
    "market"}
  for k, t in ipairs(texts) do
    local from, lt = nil, t:lower()
    for _, p in ipairs(TYPED) do
      local e
      if prompt[k] then
        e = select(2, lt:find("%f[%w]" .. p .. " %S"))
      else
        e = select(2, lt:find("^" .. p .. " %S", #t - #bare(t) + 1))
      end
      if e and (not from or e < from) then from = e end
    end
    if from then
      span(hard, k, from, #t)
      stats.typed = stats.typed + 1
      local j = k
      while j < k + 3 and #texts[j] >= 55 and not texts[j]:match('[%.!%?"~]%s*$') and texts[j + 1]
        and not prompt[j + 1] and not texts[j + 1]:match("^%s*$") and not bare(texts[j + 1]):match("^You ")
        and not bare(texts[j + 1]):match("^%(") do
        j = j + 1
        span(hard, j, 1, #texts[j])
      end
    end
  end

  -- Emotes: game messages repeat with other names, so a line starting with someone that is seen once is
  -- their own words. svof's patterns come first: such a line is kept if filler would change what matches.
  local function startsWithPerson(t)
    local off = #t - #bare(t)
    local w = bare(t):match("^(%u%l+) %l")
    return w and people[w:lower()] and off + #w
  end
  local function template(t)
    return (t:gsub("%a+", function(w) if fakes[w:lower()] then return "N" end end):gsub("%d+", "#"))
  end
  local seen = {}
  for k, t in ipairs(texts) do
    if not prompt[k] and startsWithPerson(t) then
      local key = template(bare(t))
      seen[key] = (seen[key] or 0) + 1
    end
  end
  for k, t in ipairs(texts) do
    local e = not prompt[k] and not whoLine[k] and startsWithPerson(t)
    if e and seen[template(bare(t))] == 1 then
      span(soft, k, e + 2, #t)
      stats.emotes = stats.emotes + 1
      stats.emoteLines = stats.emoteLines + 1
      local j = k
      while not texts[j]:match('[%.!%?"\')]%s*$') and j < k + 3 and texts[j + 1] and not texts[j + 1]:match("^%s*$")
        and not prompt[j + 1] and not startsWithPerson(texts[j + 1]) and not texts[j + 1]:find('"', 1, true) do
        j = j + 1
        span(soft, j, 1, #texts[j])
        stats.emoteLines = stats.emoteLines + 1
      end
    end
  end
end

-- Filler is rerolled until the line matches the same svof patterns as before; an emote that cannot be
-- filled that way is left as it is
local function scrubbedText(i)
  local t = texts[i]
  if not hard[i] and not soft[i] then
    local r = rename(t)
    if r ~= t and analyse(r) ~= analyse(t) then
      die("line " .. i .. ": renaming changes which svof patterns match it")
    end
    return r
  end
  local pins = {}
  local want = analyse(t, pins)
  local function fill(spans)
    for attempt = 1, 200 do
      local s = t
      for _, sp in ipairs(spans) do
        local a, b = sp[1], sp[2]
        s = s:sub(1, a - 1) .. reshape(s:sub(a, b), rng(seedOf(i .. ":" .. a .. ":" .. attempt)), pins, a) .. s:sub(b + 1)
      end
      s = rename(s)
      if analyse(s) == want then
        stats.rerolls = stats.rerolls + attempt - 1
        return s
      end
    end
  end
  local all = {}
  for _, s in ipairs(hard[i] or {}) do all[#all + 1] = s end
  for _, s in ipairs(soft[i] or {}) do all[#all + 1] = s end
  local s = fill(all)
  if not s and soft[i] then
    stats.kept = stats.kept + 1
    s = fill(hard[i] or {})
  end
  return s or die("line " .. i .. ": no filler matches the same svof patterns")
end

local out, stexts = {}, {}
for i, l in ipairs(lines) do
  if not scrub then
    out[i] = l
  else
    local t, pos, parts = scrubbedText(i), 1, {}
    stexts[i] = t
    for _, s in ipairs(segments(l)) do
      if s.text then
        parts[#parts + 1] = t:sub(pos, pos + #s.text - 1)
        pos = pos + #s.text
      elseif s.proto:sub(2, 2) == "\250" then
        -- GMCP values, not keys: svof reads them by key
        parts[#parts + 1] = s.proto:gsub('"([^"]*)"(%s*:?)', function(v, colon)
          if colon:find(":", 1, true) then return nil end
          return '"' .. rename(v) .. '"' .. colon
        end)
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
local config, configData = {}, {}
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
  configData[#configData + 1] = data
end

-- The audit reads the result, not the scrub's bookkeeping: no listed name in any case, and no run of
-- six words from inside quotes in the original, except where svof's own patterns hold that text
local audit = {names = 0, spans = 0, runs = 0, game = 0, allowed = {}}
if scrub then
  local left = {}
  for _, data in ipairs({scrubbed, table.concat(configData, "\n")}) do
    for w in data:gmatch("%a+") do
      local l = w:lower()
      if listed[l] then left[l] = (left[l] or 0) + 1 end
    end
  end
  for l in pairs(listed) do audit.names = audit.names + 1 end
  local leaked = {}
  for l, n in pairs(left) do leaked[#leaked + 1] = string.format("%d-letter %s name x%d", #l, listed[l], n) end
  table.sort(leaked)
  if #leaked > 0 then die("audit: listed names survive: " .. table.concat(leaked, ", ")) end

  local W, ECHO = 6, 6
  local function words(s)
    local ws = {}
    for w in s:gmatch("[%a']+") do
      w = w:lower():gsub("^'+", ""):gsub("'+$", "")
      if w ~= "" then ws[#ws + 1] = w end
    end
    return ws
  end
  local function each(ws, f)
    for s = 1, #ws - W + 1 do f(table.concat(ws, " ", s, s + W - 1)) end
  end
  -- Quoted text by its own rule, not the scrub's: a quote after a space or at the start of a line opens,
  -- one more level when already open; any other quote closes a level, and is ignored when none is open
  local quoted, outside, shortOnes = {}, {}, {}
  local depth = 0
  for k, t in ipairs(texts) do
    local parts, rest = {}, {}
    if t:match("^%s*$") then
      depth = 0
    elseif prompt[k] then
      rest[1] = t
    else
      local from, last, pos = 1, 1, 1
      while true do
        local c = t:find('"', pos, true)
        if not c then break end
        local prev, nxt = c > 1 and t:sub(c - 1, c - 1) or " ", t:sub(c + 1, c + 1)
        if prev:match("[%s%(%[{]") and nxt:match("%S") then
          if depth == 0 then
            rest[#rest + 1] = t:sub(last, c - 1)
            from = c + 1
            audit.spans = audit.spans + 1
          end
          depth = depth + 1
        elseif depth > 0 then
          depth = depth - 1
          if depth == 0 then
            parts[#parts + 1] = t:sub(from, c - 1)
            last = c + 1
          end
        end
        pos = c + 1
      end
      if depth > 0 then parts[#parts + 1] = t:sub(from) else rest[#rest + 1] = t:sub(last) end
    end
    for _, frag in ipairs(parts) do
      local ws = words(frag)
      if #ws >= W then
        each(ws, function(key)
          quoted[key] = quoted[key] or {}
          quoted[key][k] = true
        end)
      elseif select(2, frag:gsub("%a%a+", "")) >= 3 then
        shortOnes[#shortOnes + 1] = {k, frag:match("^%s*(.-)%s*$")}
      end
    end
    outside[k] = rest
  end
  -- The same words outside quotes are game text, unless just before the quote: the player's command, echoed
  -- before the game's "You say"
  local game = {}
  for k, rest in pairs(outside) do
    for _, part in ipairs(rest) do
      each(words(part), function(key)
        local lines = quoted[key]
        if lines then
          local echo = false
          for d = 0, ECHO do echo = echo or lines[k + d] or false end
          if not echo then game[key] = (game[key] or 0) + 1 end
        end
      end)
    end
  end
  for _ in pairs(quoted) do audit.runs = audit.runs + 1 end
  audit.runs = audit.runs + #shortOnes
  local svofWords = {}
  for _, p in ipairs(svof.text) do svofWords[#svofWords + 1] = table.concat(words(p), " ") end
  svofWords = "\n" .. table.concat(svofWords, "\n") .. "\n"
  local function svofHas(s)
    return svofWords:find(s, 1, true) ~= nil
  end
  local survivors, found = {}, {}
  for i, t in ipairs(stexts) do
    each(words(t), function(key)
      local lines = quoted[key]
      if not lines then return end
      if svofHas(key) then
        audit.allowed[key] = true
      elseif lines[i] then
        survivors[#survivors + 1] = "line " .. i .. " still says: " .. key
      else
        found[key] = (found[key] or 0) + 1
      end
    end)
  end
  for key, n in pairs(found) do
    if n > (game[key] or 0) then
      survivors[#survivors + 1] = string.format("seen %d times, %d as game text: %s", n, game[key] or 0, key)
    else
      audit.game = audit.game + 1
    end
  end
  for _, so in ipairs(shortOnes) do
    if stexts[so[1]]:find(so[2], 1, true) then
      if svofHas(table.concat(words(so[2]), " ")) then
        audit.allowed[so[2]] = true
      else
        survivors[#survivors + 1] = "line " .. so[1] .. " still says: " .. so[2]
      end
    end
  end
  table.sort(survivors)
  if #survivors > 0 then
    die("audit: " .. #survivors .. " quoted runs survive:\n  " .. table.concat(survivors, "\n  ", 1, math.min(#survivors, 20)))
  end
end

-- Reference runs of this corpus: a run is held to their line counts
local expect = {}
for _, path in ipairs(expects) do
  local r = {}
  for line in slurp(path):gmatch("[^\n]+") do
    local k, v = line:match("^([%w_]+) (.*)$")
    if k then r[k] = v end
  end
  local function need(k)
    return r[k] or die(path .. " has no " .. k .. "; is it a result.txt from this version of bench.lua?")
  end
  if need("corpus_md5") ~= md5 then die(path .. " is a run of another corpus") end
  if need("stop_reason") ~= "end_marker" then die(path .. " did not reach the END marker") end
  if tonumber(need("svo_prompts")) ~= prompts then die(path .. ": svof saw " .. r.svo_prompts .. " of " .. prompts .. " prompts") end
  local mode = need("mode")
  -- A package built without reference counts fails only that check; any other failure disqualifies the run
  if need("gate") ~= "pass" and need("gate") ~= "FAIL: no reference line counts for " .. mode .. " runs" then
    die(path .. " failed the gate: " .. r.gate)
  end
  if expect[mode] then die("two --expect results for " .. mode .. " mode") end
  expect[mode] = string.format("%s = {lines = %d, game_lines = %d}", mode, tonumber(need("lines")), tonumber(need("game_lines")))
end
local expectList = {}
for _, e in pairs(expect) do expectList[#expectList + 1] = e end
table.sort(expectList)

local version = tonumber(opts["bundle-version"] or "1") or die("--bundle-version takes a number")
local cfgList = {}
for _, f in ipairs(config) do cfgList[#cfgList + 1] = string.format("%q", f) end
-- Repeat runs differ by up to about a hundred lines in 141,800; 0.5% is far above that, far below a broken replay
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
  expect = {%s},
  tolerance = 0.005,
}
]], version, class, prompts, newlines, #scrubbed, md5, tostring(scrub), table.concat(cfgList, ", "),
  table.concat(expectList, ", ")))

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
if #expectList > 0 then print("reference line counts: " .. table.concat(expectList, ", ")) end
if scrub then
  local l = stats.listed
  print(string.format("names: %d listed (namedb %d, who-lists %d, --names %d, GMCP %d), %d renamed in any case",
    audit.names, l.namedb or 0, l.who or 0, l.names or 0, l.gmcp or 0, stats.renamed))
  print(string.format("scrubbed: %d quotes (%d lines carried over, %d reopened, %d never closed), %d typed, %d emotes (%d lines, %d kept for svof)",
    stats.quotes, stats.quoteLines, stats.retro, stats.unclosed, stats.typed, stats.emotes, stats.emoteLines, stats.kept))
  print(string.format("every changed line matches the same svof patterns (%d exact, begin and substring patterns, %d regexes; %d filler rerolls)",
    (function() local n = #svof.subs for _ in pairs(svof.exact) do n = n + 1 end for _, b in pairs(svof.beginsBy) do n = n + #b end return n end)(),
    #regexes, stats.rerolls))
  local allowed = {}
  for s in pairs(audit.allowed) do allowed[#allowed + 1] = s end
  table.sort(allowed)
  print(string.format("audit: 0 of %d listed names left in any case; 0 of %d quoted runs (from %d quotes) left (%d also game text), %d kept as svof's own text%s",
    audit.names, audit.runs, audit.spans, audit.game, #allowed, #allowed > 0 and ":" or ""))
  for _, s in ipairs(allowed) do print("  " .. s) end
end
