-- Builds SvofBench.mpackage, everything the svof replay benchmark needs in one file: svof, its saved
-- config, a prompted corpus scrubbed down to the text svof's triggers read, and the scripts that run it
-- (bench.lua, gate.lua, and standalone mode's loader). See README.md.
local USAGE = [[
usage: lua5.1 make-package.lua --corpus <prompted-log> --svof <svof.mpackage> --config <svo-folder>
           --out <SvofBench.mpackage> [--namedb <Database_namedb.db>] [--names <file>]
           [--expect <result.txt>]... [--bundle-version <n>] [--report <file>] [--no-scrub]
]]
local here = arg[0]:match("^(.*)/[^/]*$") or "."

local function usage(msg)
  io.stderr:write("make-package: " .. msg .. "\n" .. USAGE)
  os.exit(2)
end

local VALUED = {corpus = true, svof = true, config = true, out = true, namedb = true, names = true,
  expect = true, ["bundle-version"] = true, report = true}
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

-- Each line's text as svof's triggers see it, whether it is a prompt, and the player's name from GMCP by then
local texts, prompt, me = {}, {}, {}
local class
do
  local name
  for i, l in ipairs(lines) do
    local t = {}
    for _, s in ipairs(segments(l)) do
      if s.text then
        t[#t + 1] = s.text
      elseif s.proto == "\255\249" then
        prompt[i] = true
      elseif s.proto:sub(1, 3) == "\255\250\201" and s.proto:match("^...Char%.Status ") then
        name = s.proto:match('"name": "([^"]*)"') or name
        class = class or s.proto:match('"class": "([^"]+)"')
      end
    end
    texts[i] = table.concat(t)
    me[i] = name
  end
end
local _, prompts = corpus:gsub("\255\249", "")
local _, newlines = corpus:gsub("\n", "")
if prompts == 0 then die("the corpus has no IAC GA prompts; build it with promptify.lua") end
if not class then die("the corpus has no GMCP Char.Status with a class; build it with promptify.lua") end

local function unxml(s)
  return (s:gsub("&lt;", "<"):gsub("&gt;", ">"):gsub("&quot;", '"'):gsub("&apos;", "'"):gsub("&amp;", "&"))
end

-- svof's triggers as Mudlet runs them: the tree, each node's patterns, and how its children are reached
local SUBSTRING, PERL, BEGIN, EXACT, LUA, SPACER, COLOUR, PROMPT = 0, 1, 2, 3, 4, 5, 6, 7
local LUAPATTERN, PARAGRAPH = "luapattern", "paragraph"
local nodes, pats, literals, runtime = {}, {}, {}, {}
local svofWords = {}
do
  local p = io.popen("unzip -p " .. q(opts.svof) .. " '*.xml'")
  local xml = p:read("*a")
  p:close()
  if #xml == 0 then die("no XML in " .. opts.svof) end
  local stack, pos = {}, 1
  while true do
    local s, e, close, tag, attrs = xml:find("<(/?)(%a+)([^>]*)>", pos)
    if not s then break end
    pos = e + 1
    local empty = attrs:sub(-1) == "/"
    if tag == "Trigger" or tag == "TriggerGroup" then
      if empty then
        -- nothing to match
      elseif close == "/" then
        stack[#stack] = nil
      else
        local node = {parent = stack[#stack], pats = {}, stay = 0,
          multi = attrs:find('isMultiline="yes"', 1, true) ~= nil, filter = attrs:find('isFilterTrigger="yes"', 1, true) ~= nil}
        nodes[#nodes + 1] = node
        stack[#stack + 1] = node
      end
    elseif close == "" and not empty and stack[#stack] then
      local node = stack[#stack]
      if tag == "name" and not node.name then
        node.name = unxml(xml:match("^(.-)</name>", pos) or "")
      elseif tag == "mStayOpen" then
        node.stay = tonumber(xml:match("^%s*(%d+)", pos)) or 0
      elseif tag == "regexCodeList" then
        local listEnd = xml:find("</regexCodeList>", pos, true) or die("unexpected trigger XML in " .. opts.svof)
        local list = xml:sub(pos, listEnd - 1)
        local _, typesEnd, types = xml:find("^%s*<regexCodePropertyList>(.-)</regexCodePropertyList>", listEnd + 16)
        if not typesEnd then die("unexpected trigger XML in " .. opts.svof) end
        local strs, i = {}, 1
        while true do
          local a, b, slash = list:find("<string(/?)>", i)
          if not a then break end
          if slash == "/" then
            strs[#strs + 1] = ""
            i = b + 1
          else
            local c = list:find("</string>", b + 1, true)
            strs[#strs + 1] = unxml(list:sub(b + 1, c - 1))
            i = c + 9
          end
        end
        local kinds = {}
        for k in types:gmatch("<integer>(%-?%d+)</integer>") do kinds[#kinds + 1] = tonumber(k) end
        if #kinds ~= #strs then die("trigger " .. tostring(node.name) .. " has " .. #strs .. " patterns but " .. #kinds .. " kinds") end
        for k, str in ipairs(strs) do
          local pat = {id = #pats + 1, kind = kinds[k], text = str, node = node}
          pats[#pats + 1] = pat
          node.pats[#node.pats + 1] = pat
          for w in str:gmatch("%a+") do svofWords[w:lower()] = true end
        end
        pos = typesEnd + 1
      end
    end
  end
  -- svof also makes triggers at runtime: its defences' messages, and strings passed straight to temp triggers
  local KEYS = {on = EXACT, off = EXACT, on_only = EXACT, on_free = EXACT, off_free = EXACT, def = EXACT,
    onr = PERL, offr = PERL, defr = PERL, offs = SUBSTRING}
  -- and what it searches the lines since the prompt for
  local CALLS = {tempExactMatchTrigger = EXACT, tempRegexTrigger = PERL, tempTrigger = SUBSTRING,
    tempBeginOfLineTrigger = BEGIN, find_until_last_paragraph = PARAGRAPH, count_until_last_paragraph = PARAGRAPH}
  local SEARCHES = {substring = SUBSTRING, exact = EXACT, pattern = LUAPATTERN}
  local ESC = {n = "\n", t = "\t", r = "\r", a = "\a", b = "\b", f = "\f", v = "\v"}
  for body in xml:gmatch("<script>(.-)</script>") do
    local src, i = unxml(body), 1
    local want, depth = nil, 0
    local function found(str)
      literals[str] = true
      if want and want.kind == PARAGRAPH then
        if want.text then
          if SEARCHES[str] then runtime[#runtime + 1] = {kind = SEARCHES[str], text = want.text} end
          want = nil
        else
          want.text = str
        end
      elseif want then
        runtime[#runtime + 1] = {kind = want.kind, text = str}
        if not want.depth then want = nil end
      end
    end
    while i <= #src do
      local c = src:sub(i, i)
      if c:match("[%a_]") then
        local name = src:match("^[%w_]+", i)
        i = i + #name
        if KEYS[name] and src:match("^%s*=[^=]", i) then
          want = {kind = KEYS[name], depth = src:match("^%s*=%s*{", i) and depth + 1}
        elseif CALLS[name] and (src:match("^%s*%(", i) or src:match("^%s+or%s+[%w_]+%)%s*%(", i)) then
          want = {kind = CALLS[name]}
          i = src:find("(", i, true)
        elseif want and not want.depth then
          want = nil
        end
      elseif c == "{" or c == "}" then
        depth = depth + (c == "{" and 1 or -1)
        if want and want.depth and depth < want.depth then want = nil end
        i = i + 1
      elseif c == "-" and src:sub(i + 1, i + 1) == "-" then
        local eq = src:match("^%-%-%[(=*)%[", i)
        local close = eq and select(2, src:find("]" .. eq .. "]", i, true)) or src:find("\n", i, true)
        i = (close or #src) + 1
      elseif c == '"' or c == "'" then
        local buf, j = {}, i + 1
        while j <= #src do
          local d = src:sub(j, j)
          if d == "\\" then
            local nx = src:sub(j + 1, j + 1)
            local digits = src:match("^%d%d?%d?", j + 1)
            if digits then
              buf[#buf + 1] = string.char(tonumber(digits) % 256)
              j = j + 1 + #digits
            else
              buf[#buf + 1] = ESC[nx] or nx
              j = j + 2
            end
          elseif d == c or d == "\n" then
            break
          else
            buf[#buf + 1] = d
            j = j + 1
          end
        end
        found(table.concat(buf))
        i = j + 1
      elseif c == "[" and src:match("^%[=*%[", i) then
        local eq = src:match("^%[(=*)%[", i)
        local from = i + 2 + #eq
        local e1, e2 = src:find("]" .. eq .. "]", from, true)
        found((src:sub(from, (e1 or #src + 1) - 1):gsub("^\n", "")))
        i = (e2 or #src) + 1
      else
        i = i + 1
      end
    end
  end
  -- svof's anti-illusion limb check searches those lines for limb names it builds rather than writes out
  for _, limb in ipairs({"left arm", "right arm", "left leg", "right leg", "arms", "legs"}) do
    runtime[#runtime + 1] = {kind = SUBSTRING, text = limb}
  end
  for s in pairs(literals) do
    for w in s:gmatch("%a+") do svofWords[w:lower()] = true end
  end
end
if #pats == 0 then die("no trigger patterns in " .. opts.svof) end

-- Literal text a regex must contain wherever it matches, so most lines skip running it
local function mandatory(re)
  if re:find("(?", 1, true) and re:find("%(%?%a*[ix]") then return nil end
  if re:find("\\[xQEcpPNo0-9]") then return nil end
  local best, run, i, n, depth = "", {}, 1, #re, 0
  local function flush()
    local s = table.concat(run)
    if depth == 0 and #s > #best then best = s end
    run = {}
  end
  while i <= n do
    local c = re:sub(i, i)
    if c == "\\" then
      local e = re:sub(i + 1, i + 1)
      if e:match("%w") then flush() elseif depth == 0 then run[#run + 1] = e end
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
      depth = depth + 1
      i = i + 1
      if re:sub(i, i) == "?" then i = i + 1 end
    elseif c == ")" then
      flush()
      depth = depth - 1
      i = i + 1
    elseif c == "|" then
      if depth == 0 then return nil end
      flush()
      i = i + 1
    elseif c == "?" or c == "*" or c == "{" or c == "+" then
      if c ~= "+" then run[#run] = nil end
      flush()
      i = c == "{" and (re:find("}", i, true) or n) + 1 or i + 1
      if re:sub(i, i) == "?" or re:sub(i, i) == "+" then i = i + 1 end
    elseif c == "^" or c == "$" or c == "." then
      flush()
      i = i + 1
    else
      if depth == 0 then run[#run + 1] = c else flush() end
      i = i + 1
    end
  end
  flush()
  return best ~= "" and best or nil
end

-- The text an anchored regex's match starts with
local function prefixOf(re)
  if re:sub(1, 1) ~= "^" or not mandatory(re) then return nil end
  local run, i = {}, 2
  while i <= #re do
    local c = re:sub(i, i)
    if c == "\\" then
      local e = re:sub(i + 1, i + 1)
      if e:match("%w") or e == "" then break end
      run[#run + 1] = e
      i = i + 2
    elseif c:match("[%[%(%.%*%+%?{|%$%^]") then
      if c == "?" or c == "*" or c == "{" then run[#run] = nil end
      break
    else
      run[#run + 1] = c
      i = i + 1
    end
  end
  return #run > 0 and table.concat(run) or nil
end

local rex, PCRE_FLAGS
local function compile(re)
  local ok, compiled = pcall(rex.new, re, PCRE_FLAGS)
  return ok and compiled or nil
end

-- A Lua pattern is evaluated when its result depends only on the line, the prompt and the player's name;
-- one that reads svof's state or GMCP is assumed to fire, and is listed when it reads the line too
local SANDBOX = {string = string, tostring = tostring, tonumber = tonumber, type = type}
local function luaKind(code)
  local reads = {}
  for ref in code:gmatch("svo%.([%w_%.]+)") do
    if ref ~= "me.name" and ref ~= "paragraph_length" then reads[#reads + 1] = ref end
  end
  if #reads > 0 or code:find("gmcp", 1, true) then return "state" end
  local fn = loadstring(code)
  if not fn then return "state" end
  local svoT = {me = {}, paragraph_length = 0}
  local env = setmetatable({svo = svoT}, {__index = function(_, k)
    if SANDBOX[k] ~= nil then return SANDBOX[k] end
    error("reads " .. k)
  end})
  setfenv(fn, env)
  local function eval(line, isPrompt, name)
    env.line = line
    env.isPrompt = function() return isPrompt end
    svoT.me.name = name or ""
    svoT.paragraph_length = 0
    local ok, r = pcall(fn)
    if not ok then return nil end
    return r and true or false
  end
  for _, sample in ipairs({"", "You eat a piece of kelp.", "Someone"}) do
    if eval(sample, true, "Someone") == nil or eval(sample, false, "Someone") == nil then return "state" end
  end
  if code:find("%f[%w_]line%f[^%w_]") then return "line", eval end
  return "prompt", eval("", true, ""), eval("", false, "")
end

local signature, describe, unevaluated
if scrub then
  local ok
  ok, rex = pcall(require, "rex_pcre")
  if not ok then die("scrubbing needs lrexlib-pcre (rex_pcre) to run svof's regexes") end
  local f = rex.flags()
  PCRE_FLAGS = f.UTF8 + f.UCP
  unevaluated = {}

  -- Where each node runs: on the line, gated by the nearest ancestor that must match on this same line
  -- (one that stays open or needs several lines may fire on any line, so it passes its reach down), or on
  -- the captures of a filter
  for _, node in ipairs(nodes) do
    local anc = node.parent
    while anc and #anc.pats == 0 do anc = anc.parent end
    if not anc then
      node.group = 0
    elseif anc.filter then
      node.group, node.gate = anc, nil
    elseif anc.multi or anc.stay > 0 then
      node.group, node.gate = anc.group, anc.gate
    else
      node.group, node.gate = anc.group, anc
    end
  end

  local function newGroup()
    return {exact = {}, begins = {}, subs = {}, regexes = {}, anchored = {}, luas = {}, names = {}, filters = {}}
  end
  local groups = {[0] = newGroup()}
  for _, node in ipairs(nodes) do
    if node.group ~= 0 and not groups[node.group] then groups[node.group] = newGroup() end
  end
  local function add(g, pat)
    local k = pat.kind
    if k == EXACT then
      g.exact[pat.text] = g.exact[pat.text] or {}
      table.insert(g.exact[pat.text], pat)
    elseif k == BEGIN then
      local b = pat.text:byte(1) or 0
      g.begins[b] = g.begins[b] or {}
      table.insert(g.begins[b], pat)
    elseif k == SUBSTRING then
      g.subs[#g.subs + 1] = pat
    elseif k == PERL then
      pat.re = compile(pat.text)
      if pat.re then
        pat.need, pat.prefix = mandatory(pat.text), prefixOf(pat.text)
        if pat.prefix then
          local b = pat.prefix:byte(1)
          g.anchored[b] = g.anchored[b] or {}
          table.insert(g.anchored[b], pat)
        else
          g.regexes[#g.regexes + 1] = pat
        end
      end
    elseif k == LUA then
      local kind, a, b = luaKind(pat.text)
      if pat.text:gsub("%s+", " ") == "return line:find(svo.me.name, 1, true) and true or false" then
        g.names[#g.names + 1] = pat
      elseif kind == "line" then
        pat.eval = a
        g.luas[#g.luas + 1] = pat
      elseif kind == "prompt" then
        pat.onPrompt, pat.offPrompt = a, b
      else
        pat.always = true
        if pat.text:find("%f[%w_]line%f[^%w_]") then unevaluated[#unevaluated + 1] = pat end
      end
    elseif k == LUAPATTERN then
      pat.eval = function(hay) return hay:find(pat.text) ~= nil end
      g.luas[#g.luas + 1] = pat
    elseif k == PROMPT then
      pat.onPrompt, pat.offPrompt = true, false
    else
      pat.always = true
    end
  end
  for _, pat in ipairs(pats) do add(groups[pat.node.group], pat) end
  for _, node in ipairs(nodes) do
    if node.filter then table.insert(groups[node.group].filters, node) end
  end
  local made, seen = {name = "svof's runtime triggers", group = 0}, {}
  for _, r in ipairs(runtime) do
    local key = r.kind .. ":" .. r.text
    if not seen[key] and (r.kind ~= PERL or compile(r.text)) and (r.kind ~= LUAPATTERN or pcall(string.find, "", r.text)) then
      seen[key] = true
      local pat = {id = #pats + 1, kind = r.kind, text = r.text, node = made}
      pats[#pats + 1] = pat
      add(groups[0], pat)
    end
  end

  -- The patterns that match, those of children only where their parent lets them run
  local function evalGroup(g, hay, isPrompt, name, out, prefix)
    local hit = {}
    for _, pat in ipairs(g.exact[hay] or {}) do hit[pat] = true end
    for _, pat in ipairs(g.begins[hay:byte(1) or 0] or {}) do
      if hay:sub(1, #pat.text) == pat.text then hit[pat] = true end
    end
    for _, pat in ipairs(g.subs) do
      if hay:find(pat.text, 1, true) then hit[pat] = true end
    end
    for _, pat in ipairs(g.anchored[hay:byte(1) or 0] or {}) do
      if hay:sub(1, #pat.prefix) == pat.prefix and hay:find(pat.need, 1, true) and pat.re:exec(hay) then hit[pat] = true end
    end
    for _, pat in ipairs(g.regexes) do
      if (not pat.need or hay:find(pat.need, 1, true)) and pat.re:exec(hay) then hit[pat] = true end
    end
    if name and name ~= "" and #g.names > 0 and hay:find(name, 1, true) then
      for _, pat in ipairs(g.names) do hit[pat] = true end
    end
    for _, pat in ipairs(g.luas) do
      if pat.eval(hay, isPrompt, name) then hit[pat] = true end
    end
    local fires = {}
    local function fired(node)
      if not node then return true end
      local f = fires[node]
      if f == nil then
        f = node.multi or node.stay > 0
        if not f then
          for _, pat in ipairs(node.pats) do
            if hit[pat] or pat.always or (isPrompt and pat.onPrompt) or (not isPrompt and pat.offPrompt) then
              f = true
              break
            end
          end
        end
        f = f and fired(node.gate)
        fires[node] = f
      end
      return f
    end
    local ids = {}
    for pat in pairs(hit) do
      if fired(pat.node.gate) then ids[#ids + 1] = pat.id end
    end
    table.sort(ids)
    for _, id in ipairs(ids) do out[#out + 1] = prefix .. id end
    for _, node in ipairs(g.filters) do
      if fired(node) then
        for _, pat in ipairs(node.pats) do
          if hit[pat] and pat.kind == PERL then
            local caps = {select(3, pat.re:tfind(hay))}
            caps = caps[1] or {}
            for ci = 1, #caps do
              if type(caps[ci]) == "string" and caps[ci] ~= "" then
                evalGroup(groups[node], caps[ci], isPrompt, name, out, prefix .. pat.id .. ":" .. ci .. ">")
              end
            end
          end
        end
      end
    end
  end
  function signature(text, isPrompt, name)
    local out = {}
    evalGroup(groups[0], text, isPrompt, name, out, "")
    return table.concat(out, " ")
  end
  function describe(entry)
    local id = tonumber(entry:match("^(%d+)"))
    local node, names = pats[id].node, {}
    while node and node.name do
      table.insert(names, 1, node.name)
      node = node.parent
    end
    while #names > 2 and (names[1] == "svo (aliases, triggers)" or names[1] == "svo") do table.remove(names, 1) end
    return table.concat(names, "/")
  end
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
local function reshape(s, roll)
  return (s:gsub("%w", function(c)
    local pool = c:match("%d") and DIGITS or c:lower():match("[aeiouy]") and VOWELS or CONSONANTS
    local k = roll(#pool)
    local r = pool:sub(k, k)
    return c:match("%u") and r:upper() or r
  end))
end
local function caseLike(model, s)
  return (s:gsub("()(%a)", function(c, ch) return model:sub(c, c):match("%u") and ch:upper() or nil end))
end

-- Without the log's timestamp, or the ">" a line after a prompt can start with
local function bare(t)
  return (t:gsub("^>", ""):gsub("^%d%d:%d%d:%d%d%.%d+ ", ""))
end

local stats = {listed = {}, filled = 0, kept = 0, pinned = 0, renamed = 0, rerolls = 0, backoff = 0, nameLetters = 0,
  nameSwaps = 0, trimmed = 0, captured = 0, pseudo = 0, pseudoWords = 0, svofVocab = 0, gameVocab = 0, categories = {}}
local listed, fakes, taken = {}, {}, {}
local function fakeOf(w)
  local l = w:lower()
  local f = fakes[l]
  if not f then
    local roll, tries = rng(seedOf(l)), 0
    repeat
      f = reshape(l, roll)
      tries = tries + 1
      if tries > 1000 then die("no free stand-in for a " .. #l .. "-letter word") end
    -- One- and two-letter words run out of unused stand-ins; a common one is no clue to a name
    until not listed[f] and (not taken[f] or (#l <= 2 and tries > 20 and not listed[l]))
    taken[f] = true
    fakes[l] = f
  end
  return caseLike(w, f)
end
-- svof's timings and illusion marks in the recorded log: the gate strips exactly these, so they stay
local MARKS = {"%(%d+%.%d+s%)", "%(%d+s%)", "%(%?s%)", "%(!?i%)"}
local function marks(s)
  local spans = {}
  for _, m in ipairs(MARKS) do
    for a, b in s:gmatch("()" .. m .. "()") do spans[#spans + 1] = {a, b - 1} end
  end
  return spans
end
-- Same shape, other letters: a word that comes out as itself or as a listed name is rolled again
local function filler(s, key)
  local f = reshape(s, rng(seedOf(key))):gsub("()(%a+)", function(pos, w)
    local was, n = s:sub(pos, pos + #w - 1):lower(), 0
    while listed[w:lower()] or w:lower() == was do
      n = n + 1
      if n > 100 then die("no filler for a " .. #w .. "-letter word") end
      w = reshape(w, rng(seedOf(key .. ":" .. pos .. ":" .. n)))
    end
    return w
  end)
  for _, sp in ipairs(marks(s)) do f = f:sub(1, sp[1] - 1) .. s:sub(sp[1], sp[2]) .. f:sub(sp[2] + 1) end
  return f
end

local function tokens(t)
  local list2 = {}
  for s, w in t:gmatch("()(%a+)") do list2[#list2 + 1] = {s = s, e = s + #w - 1, w = w, alpha = true} end
  for s, w in t:gmatch("()(%d+)") do list2[#list2 + 1] = {s = s, e = s + #w - 1, w = w} end
  table.sort(list2, function(a, b) return a.s < b.s end)
  return list2
end

local out, stexts = {}, {}
local entryCaps, inSpans, vocab
local meFake = {}
if scrub then
  for _, t in ipairs(texts) do
    for w in t:gmatch("%a+") do taken[w:lower()] = true end
  end
  for w in pairs(svofWords) do taken[w] = true end

  -- Real names, for renaming the ones svof's text holds on to, and for the audit: svof's name database,
  -- who-lists in the corpus, --names and GMCP
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
    local res = p:read("*a")
    local ok = p:close()
    if (ok ~= 0 and ok ~= true) or res:find("Error") then die("cannot read names from " .. opts.namedb .. ": " .. res) end
    for w in res:gmatch("[^\n]+") do
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
  -- QW lists ("Abysal, Aedesynne, ..., and Zriek." then "(81 total)"), svof's QW by city, and CWHO rows
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
  local i = 1
  while i <= #texts do
    local t = bare(texts[i])
    if not prompt[i] and t:match("^%(svof%): qw") then
      local j = nextText(i + 1)
      while j <= #texts and listNames(texts[j]) do
        for _, w in ipairs(listNames(texts[j])) do list(w, "who") end
        j = nextText(j + 1)
      end
      i = j
    elseif not prompt[i] and t:match("^Citizen%s+Rank%s+CT%s+Class%s*$") then
      local j = nextText(i + 1)
      if bare(texts[j] or ""):match("^%-+%s") then j = nextText(j + 1) end
      while j <= #texts do
        local col = bare(texts[j]):match("^(%u.-)%s%s+%d+%s+O[nf]f?%s")
        if not col then break end
        for w in col:gmatch("%u%l+") do
          if not svofWords[w:lower()] then list(w, "who") end
        end
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
        end
      end
      i = j
    else
      i = i + 1
    end
  end
  local sortedListed = {}
  for l in pairs(listed) do sortedListed[#sortedListed + 1] = l end
  table.sort(sortedListed)
  for _, l in ipairs(sortedListed) do fakeOf(l) end
  for i2 = 1, #texts do meFake[i2] = me[i2] and fakeOf(me[i2]) end

  -- Every line is filler, unless that changes which of svof's patterns match it. Then svof's own text
  -- is kept: a word stays only if filler in its place changes the match or what a pattern captures, names
  -- get the same stand-in everywhere, and anything else is filler
  local function splice(t, toks, repl)
    local parts, pos = {}, 1
    for k, tok in ipairs(toks) do
      if repl[k] then
        parts[#parts + 1] = t:sub(pos, tok.s - 1)
        parts[#parts + 1] = repl[k]
        pos = tok.e + 1
      end
    end
    parts[#parts + 1] = t:sub(pos)
    return table.concat(parts)
  end
  -- Where in t the svof matches that got lost were: the letters a renamed name has to keep
  local function lostSpans(t, want, got)
    local have, spans = {}, {}
    for e in got:gmatch("%S+") do have[e] = true end
    for e in want:gmatch("%S+") do
      if not have[e] then
        local id, ci = e:match("^(%d+):?(%d*)")
        local pat = pats[tonumber(id)]
        local a, b
        if pat.kind == SUBSTRING then
          a, b = t:find(pat.text, 1, true)
        elseif pat.kind == BEGIN then
          a, b = 1, #pat.text
        elseif pat.kind == EXACT then
          a, b = 1, #t
        elseif pat.kind == PERL and pat.re then
          local s0, e0, offs = pat.re:exec(t)
          a, b = s0, e0
          if ci ~= "" and offs and offs[2 * ci - 1] then a, b = offs[2 * ci - 1], offs[2 * ci] end
        end
        if not a then return nil end
        spans[#spans + 1] = {a, b}
      end
    end
    return spans
  end
  local function keepSpans(t, toks, repl, spans)
    for k, r in pairs(repl) do
      local tok, chars = toks[k], {}
      for pos = tok.s, tok.e do
        local c = r:sub(pos - tok.s + 1, pos - tok.s + 1)
        for _, sp in ipairs(spans) do
          if pos >= sp[1] and pos <= sp[2] then c = t:sub(pos, pos) end
        end
        chars[#chars + 1] = c
      end
      repl[k] = table.concat(chars)
    end
  end
  -- Spans of an entry's match and captures in t, which svof's scripts read; "id:ci>id" is a filter's child
  function entryCaps(t, e)
    local segs, base, hay = {}, 0, t
    for seg in e:gmatch("[^>]+") do segs[#segs + 1] = seg end
    for k = 1, #segs - 1 do
      local id, ci = segs[k]:match("^(%d+):(%d+)$")
      local _, _, offs = pats[tonumber(id)].re:exec(hay)
      ci = tonumber(ci)
      local a, b = offs and offs[2 * ci - 1], offs and offs[2 * ci]
      if not a then return {} end
      hay, base = hay:sub(a, b), base + a - 1
    end
    local pat, caps = pats[tonumber(segs[#segs])], {}
    if pat.kind == PERL and pat.re then
      local s0, e0, offs = pat.re:exec(hay)
      if s0 then caps[1] = {base + s0, base + e0} end
      for c = 1, offs and #offs or 0, 2 do
        if offs[c] then caps[#caps + 1] = {base + offs[c], base + offs[c + 1]} end
      end
    end
    return caps
  end
  local function captureSig(s, entries)
    local parts = {}
    for _, e in ipairs(entries) do
      for _, c in ipairs(entryCaps(s, e)) do parts[#parts + 1] = s:sub(c[1], c[2]) end
      parts[#parts + 1] = "\0"
    end
    return table.concat(parts, "\1")
  end
  function inSpans(spans, a, b)
    for _, sp in ipairs(spans) do
      if a <= sp[2] and b >= sp[1] then return true end
    end
    return false
  end

  local want, lostOf, kept = {}, {}, {}
  for i2, t in ipairs(texts) do
    local p, fake = prompt[i2] or false, meFake[i2]
    local w, first = signature(t, p, me[i2]), nil
    for attempt = 1, 4 do
      local f = filler(t, i2 .. ":" .. attempt)
      first = first or f
      if signature(f, p, fake) == w then
        stexts[i2] = f
        stats.rerolls = stats.rerolls + attempt - 1
        break
      end
    end
    if stexts[i2] then
      stats.filled = stats.filled + 1
    else
      -- What makes it svof's: the matches filler loses
      local have, lost = {}, {}
      for e in signature(first, p, fake):gmatch("%S+") do have[e] = true end
      for e in w:gmatch("%S+") do
        if not have[e] then lost[#lost + 1] = e end
      end
      want[i2], lostOf[i2] = w, lost
      kept[#kept + 1] = i2
    end
  end

  -- svof reads what it matches and captures, so there a word stays if svof's own text or the game's uses it
  local GAME_CONTEXTS = 3
  vocab = {}
  do
    local p = io.popen("unzip -p " .. q(opts.svof))
    for w in p:read("*a"):gmatch("%a+") do vocab[w:lower()] = true end
    p:close()
    for _ in pairs(vocab) do stats.svofVocab = stats.svofVocab + 1 end
    local function context(s)
      return (s:gsub("%d+", "N"):gsub("%a+", function(x) return listed[x:lower()] and "@" or nil end))
    end
    local seen = {}
    for _, i2 in ipairs(kept) do
      local t = bare(texts[i2])
      for pos, w in t:gmatch("()(%a+)") do
        local l = w:lower()
        if not vocab[l] and not listed[l] then
          local r = seen[l] or {n = 0, lower = 0, ctx = {}, nctx = 0}
          seen[l] = r
          r.n = r.n + 1
          if w == l then r.lower = r.lower + 1 end
          local key = context(t:sub(1, pos - 1)) .. "_" .. context(t:sub(pos + #w))
          if not r.ctx[key] then
            r.ctx[key] = true
            r.nctx = r.nctx + 1
          end
        end
      end
    end
    local game = {}
    for l, r in pairs(seen) do
      if r.lower * 2 >= r.n and r.nctx >= GAME_CONTEXTS then game[#game + 1] = l end
    end
    for _, l in ipairs(game) do vocab[l] = true end
    stats.gameVocab, stats.gameContexts = #game, GAME_CONTEXTS
  end
  local capSpans, pseudo = {}, {}
  for _, i2 in ipairs(kept) do
    local t, spans = texts[i2], {}
    for _, e in ipairs(lostOf[i2]) do
      for _, c in ipairs(entryCaps(t, e)) do spans[#spans + 1] = c end
    end
    capSpans[i2] = spans
    for pos, w in t:gmatch("()(%a+)") do
      local l = w:lower()
      if not vocab[l] and not listed[l] and inSpans(spans, pos, pos + #w - 1) and not pseudo[l] then
        pseudo[l] = true
        stats.pseudoWords = stats.pseudoWords + 1
      end
    end
  end

  local reportLines = {}
  for _, i2 in ipairs(kept) do
    local t, p, fake, w, lost = texts[i2], prompt[i2] or false, meFake[i2], want[i2], lostOf[i2]
    local result
    local toks = tokens(t)
    local names, fixed, repl, markSpans = {}, {}, {}, marks(t)
    for k, tok in ipairs(toks) do
      local l = tok.w:lower()
      if tok.alpha and (listed[l] or pseudo[l]) then
        names[k] = true
        repl[k] = fakeOf(tok.w)
      elseif inSpans(markSpans, tok.s, tok.e) then
        fixed[k] = "mark"
      elseif (not tok.alpha or vocab[l]) and inSpans(capSpans[i2], tok.s, tok.e) then
        fixed[k] = "capture"
      end
    end
    local base = splice(t, toks, repl)
    -- A name svof's pattern runs into keeps the letters it matches; a stand-in that matches too much is
    -- swapped for another on this line only
    for attempt = 1, 8 do
      local got = signature(base, p, fake)
      if got == w then break end
      local spans = lostSpans(t, w, got)
      if spans and #spans > 0 then
        keepSpans(t, toks, repl, spans)
        base = splice(t, toks, repl)
        if signature(base, p, fake) == w then
          stats.nameLetters = stats.nameLetters + 1
          break
        end
      end
      if attempt == 8 then die("line " .. i2 .. ": renaming a listed name changes which svof patterns match it") end
      for k in pairs(names) do
        local l, f2, tries = toks[k].w:lower(), nil, 0
        local roll = rng(seedOf(l .. ":" .. i2 .. ":" .. attempt))
        repeat
          f2 = reshape(l, roll)
          tries = tries + 1
        until (not taken[f2] and not listed[f2]) or tries > 1000
        repl[k] = caseLike(toks[k].w, f2)
      end
      stats.nameSwaps = stats.nameSwaps + 1
      base = splice(t, toks, repl)
    end
    local capWant = captureSig(base, lost)
    local function ok(s)
      return signature(s, p, fake) == w and captureSig(s, lost) == capWant
    end
    local function fillTok(k, attempt)
      return filler(toks[k].w, i2 .. ":" .. k .. ":" .. attempt)
    end
    local function variant(attempt, which)
      local r = {}
      for k, v in pairs(repl) do r[k] = v end
      for k in pairs(which) do r[k] = fillTok(k, attempt) end
      return splice(t, toks, r)
    end
    local cand = {}
    for k in ipairs(toks) do
      if not names[k] and not fixed[k] then cand[k] = true end
    end
    local variable = {}
    if ok(variant(1, cand)) then
      variable = cand
    else
      for k in ipairs(toks) do
        if cand[k] then
          local one = {[k] = true}
          if ok(variant(1, one)) and ok(variant(2, one)) then variable[k] = true end
        end
      end
    end
    local function build(attempt, spare)
      local r = {}
      for k, v in pairs(repl) do r[k] = v end
      for k in pairs(variable) do
        if not spare[k] then
          local tok = toks[k]
          r[k] = tok.alpha and tok.w:match("^%u%a%a") and attempt == 1 and fakeOf(tok.w) or fillTok(k, attempt)
        end
      end
      return splice(t, toks, r), r
    end
    local spare, final = {}, nil
    for attempt = 1, 4 do
      local s, r = build(attempt, spare)
      if ok(s) then
        result, final = s, r
        break
      end
    end
    -- Words svof can do without one at a time but not all together: fill them one by one from the
    -- line as it was, and keep each one whose filler changes the match
    if not result then
      local r = {}
      for k, v in pairs(repl) do r[k] = v end
      for k in ipairs(toks) do
        if variable[k] then
          r[k] = fillTok(k, 1)
          if not ok(splice(t, toks, r)) then
            r[k] = nil
            spare[k] = true
            stats.backoff = stats.backoff + 1
          end
        end
      end
      result, final = splice(t, toks, r), r
    end
    -- A word kept for a pattern that matches only part of it keeps just that part
    for k, tok in ipairs(toks) do
      if not names[k] and not fixed[k] and (not variable[k] or spare[k]) and #tok.w > 1 then
        local probe = {}
        for k2, v in pairs(final) do probe[k2] = v end
        probe[k] = fillTok(k, 1)
        local spans = lostSpans(result, w, signature(splice(t, toks, probe), p, fake))
        if spans and #spans > 0 then
          local one = {[k] = probe[k]}
          keepSpans(t, toks, one, spans)
          if one[k] ~= tok.w and not listed[one[k]:lower()] then
            probe[k] = one[k]
            local s = splice(t, toks, probe)
            if ok(s) then
              result, final = s, probe
              stats.trimmed = stats.trimmed + 1
            end
          end
        end
      end
    end
    stats.kept = stats.kept + 1
    for k, tok in ipairs(toks) do
      if names[k] then
        if listed[tok.w:lower()] then stats.renamed = stats.renamed + 1 else stats.pseudo = stats.pseudo + 1 end
      elseif fixed[k] then
        if fixed[k] == "capture" then stats.captured = stats.captured + 1 end
      elseif not variable[k] or spare[k] then
        stats.pinned = stats.pinned + 1
      end
    end
    local cat = lost[1] and describe(lost[1]) or "renamed only"
    local c = stats.categories[cat]
    if not c then
      c = {n = 0, example = result}
      stats.categories[cat] = c
    end
    c.n = c.n + 1
    reportLines[#reportLines + 1] = i2
    stexts[i2] = result
  end
  stats.reportLines = reportLines
end

-- GMCP: keys and vitals stay; a name gets its stand-in, and every other value is filler, except the
-- Char.Status fields svof reads that many players share
local STATUS_KEEP = {class = true, race = true, level = true, gender = true, specialisation = true}
local function scrubGmcp(seg, i, n)
  local pkg, body = seg:match("^\255\250\201(%S+)(.-)\255\240$")
  if not pkg or pkg == "Char.Vitals" then return seg end
  local k = 0
  local function fill(s)
    k = k + 1
    return filler(s, i .. ":gmcp:" .. n .. ":" .. k)
  end
  local parts, pos, key = {}, 1, nil
  while pos <= #body do
    local s, e, v, colon = body:find('^"([^"]*)"(%s*:?)', pos)
    if s then
      local new = v
      if colon:find(":", 1, true) then
        key = v
      elseif pkg == "Char.Status" and STATUS_KEEP[key] then
        new = v
      elseif key == "name" or key == "fullname" then
        new = v:gsub("%a+", function(w) return listed[w:lower()] and fakeOf(w) or fill(w) end):gsub("%d+", fill)
      else
        new = fill(v)
      end
      parts[#parts + 1] = '"' .. new .. '"' .. colon
      pos = e + 1
    else
      local stop = body:find('"', pos, true) or #body + 1
      parts[#parts + 1] = body:sub(pos, stop - 1):gsub("%d+", fill)
      pos = stop
    end
  end
  return "\255\250\201" .. pkg .. table.concat(parts) .. "\255\240"
end

for i, l in ipairs(lines) do
  if not scrub then
    out[i] = l
  else
    local t, pos, parts, n = stexts[i], 1, {}, 0
    for _, s in ipairs(segments(l)) do
      if s.text then
        parts[#parts + 1] = t:sub(pos, pos + #s.text - 1)
        pos = pos + #s.text
      elseif s.proto:sub(2, 2) == "\250" then
        n = n + 1
        parts[#parts + 1] = scrubGmcp(s.proto, i, n)
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
  if scrub then
    -- An exported priority list is stamped with its author and the time; svof reads neither
    data = data:gsub("^(%-%- Priorities list '[^'\n]*') by [^\n]*", "%1")
  end
  run("mkdir -p " .. q(stage .. "/svo/" .. f:match("^(.*)/")))
  spit(stage .. "/svo/" .. f, data)
  configData[#configData + 1] = data
end

-- The audit reads the result, not the scrub's bookkeeping
local audit = {names = 0, runs = 0, shared = 0}
if scrub then
  -- Every line matches the same svof patterns as before
  for i, t in ipairs(stexts) do
    if signature(t, prompt[i] or false, meFake[i]) ~= signature(texts[i], prompt[i] or false, me[i]) then
      die("audit: line " .. i .. " matches other svof patterns than it did")
    end
  end
  -- No listed name in any case, in the corpus, GMCP included, or the config
  local left = {}
  for _, data in ipairs({scrubbed, table.concat(configData, "\n")}) do
    for w in data:gmatch("%a+") do
      local l = w:lower()
      if listed[l] then left[l] = (left[l] or 0) + 1 end
    end
  end
  for _ in pairs(listed) do audit.names = audit.names + 1 end
  local leaked = {}
  for l, n in pairs(left) do leaked[#leaked + 1] = string.format("%d-letter %s name x%d", #l, listed[l], n) end
  table.sort(leaked)
  if #leaked > 0 then die("audit: listed names survive: " .. table.concat(leaked, ", ")) end

  -- The game's fixed messages appear in filler lines too, so a run may stay where svof needs all its words
  local W = 4
  local function unmarked(s)
    for _, m in ipairs(MARKS) do s = s:gsub(m, function(x) return (" "):rep(#x) end) end
    return s
  end
  local function words(s)
    local ws = {}
    for w in unmarked(s):gmatch("%a+") do ws[#ws + 1] = w:lower() end
    return ws
  end
  local function each(ws, f)
    for s = 1, #ws - W + 1 do f(table.concat(ws, " ", s, s + W - 1), s) end
  end
  local function needed(i, t, from)
    local want, n = signature(t, prompt[i] or false, meFake[i]), 0
    local have, spans = {}, {}
    for e in signature(filler(t, "audit:" .. i), prompt[i] or false, meFake[i]):gmatch("%S+") do have[e] = true end
    for e in want:gmatch("%S+") do
      if not have[e] then
        for _, c in ipairs(entryCaps(t, e)) do spans[#spans + 1] = c end
      end
    end
    for pos, w in unmarked(t):gmatch("()(%a+)") do
      n = n + 1
      if n >= from and n < from + W then
        local changed = vocab[w:lower()] and inSpans(spans, pos, pos + #w - 1)
        for attempt = 1, changed and 0 or 4 do
          local f = reshape(w, rng(seedOf("audit:" .. i .. ":" .. pos .. ":" .. attempt)))
          local u = t:sub(1, pos - 1) .. f .. t:sub(pos + #w)
          if f:lower() ~= w:lower() and signature(u, prompt[i] or false, meFake[i]) ~= want then
            changed = true
            break
          end
        end
        if not changed then return false end
      end
    end
    return true
  end
  local runs = {}
  local keptSet = {}
  for _, k in ipairs(stats.reportLines) do keptSet[k] = true end
  for i, t in ipairs(texts) do
    if not keptSet[i] then each(words(t), function(key) runs[key] = true end) end
  end
  for _ in pairs(runs) do audit.runs = audit.runs + 1 end
  local survivors, shared = {}, {}
  for i, t in ipairs(stexts) do
    each(words(t), function(key, from)
      if runs[key] then
        if keptSet[i] and needed(i, t, from) then
          shared[key] = true
        else
          survivors[#survivors + 1] = "line " .. i .. ": " .. key
        end
      end
    end)
  end
  for _ in pairs(shared) do audit.shared = audit.shared + 1 end
  if #survivors > 0 then
    die("audit: " .. #survivors .. " four-word runs from filler lines survive:\n  " .. table.concat(survivors, "\n  ", 1, math.min(#survivors, 20)))
  end
end

-- Reference runs of this corpus: a run is held to their counts
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
  if need("gate") ~= "pass" and need("gate") ~= "FAIL: no reference counts for " .. mode .. " runs" then
    die(path .. " failed the gate: " .. r.gate)
  end
  if expect[mode] then die("two --expect results for " .. mode .. " mode") end
  expect[mode] = string.format("%s = {lines = %d, game_lines = %d, added_lines = %d}", mode, tonumber(need("lines")),
    tonumber(need("game_lines")), tonumber(need("added_lines")))
end
local expectList = {}
for _, e in pairs(expect) do expectList[#expectList + 1] = e end
table.sort(expectList)

-- How far a run may stray from its reference: twice the widest spread seen on one machine, 18 lines, 9 game
-- lines and 16 added lines, over 41 socket runs of a scrubbed package, 58 of its raw log and 5 feed runs.
-- One more socket run, on a machine under heavy load, had 140 more added lines: svof's timers fired late
local SLACK = {lines = 36, game_lines = 18, added_lines = 32}

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
  expect = {%s},
  slack = {lines = %d, game_lines = %d, added_lines = %d},
}
]], version, class, prompts, newlines, #scrubbed, md5, tostring(scrub), table.concat(cfgList, ", "),
  table.concat(expectList, ", "), SLACK.lines, SLACK.game_lines, SLACK.added_lines))

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
if #expectList > 0 then print("reference counts: " .. table.concat(expectList, ", ")) end
if scrub then
  local l = stats.listed
  local cats = {}
  for name, c in pairs(stats.categories) do cats[#cats + 1] = {name = name, n = c.n, example = c.example} end
  table.sort(cats, function(a, b) return a.n > b.n or (a.n == b.n and a.name < b.name) end)
  print(string.format("names: %d listed (namedb %d, who-lists %d, --names %d, GMCP %d)",
    audit.names, l.namedb or 0, l.who or 0, l.names or 0, l.gmcp or 0))
  print(string.format("lines: %d filler, %d kept for svof (%d words svof matches on, %d of them cut to the part it matches, %d names renamed, %d words kept as a last resort), %d rerolls",
    stats.filled, stats.kept, stats.pinned, stats.trimmed, stats.renamed, stats.backoff, stats.rerolls))
  print(string.format("what svof matches and captures: %d words and numbers kept, %d other words given stand-ins (%d distinct); vocabulary: %d words of svof's text and %d game words in at least %d svof-matched messages",
    stats.captured, stats.pseudo, stats.pseudoWords, stats.svofVocab, stats.gameVocab, stats.gameContexts))
  print(string.format("renamed names: %d lines keep letters a pattern matches inside a name, %d use another stand-in",
    stats.nameLetters, stats.nameSwaps))
  print(string.format("audit: every line matches the same svof patterns; 0 of %d listed names left in any case; of %d four-word runs from filler lines, none left except %d in lines svof needs every word of",
    audit.names, audit.runs, audit.shared))
  if #unevaluated > 0 then
    local u = {}
    for _, pat in ipairs(unevaluated) do u[#u + 1] = describe(tostring(pat.id)) end
    print(string.format("assumed to fire (they read svof's state as well as the line): %s", table.concat(u, ", ")))
  end
  print(string.format("kept lines by the svof trigger that needs them (%d triggers%s):", #cats,
    opts.report and ", every one in " .. opts.report or ", top 25; --report lists all"))
  local rep = {}
  for k, c in ipairs(cats) do
    local row = string.format("%6d  %s", c.n, c.name)
    if k <= 25 then print(row) end
    rep[#rep + 1] = row .. "\n        " .. c.example
  end
  if opts.report then spit(opts.report, table.concat(rep, "\n") .. "\n") end
end
