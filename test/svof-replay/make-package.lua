-- Builds SvofBench.mpackage, everything the svof replay benchmark needs in one file: svof, its saved
-- config, a prompted corpus scrubbed down to the text svof's triggers read, and the scripts that run it
-- (bench.lua, gate.lua, and standalone mode's loader). See README.md.
local USAGE = [[
usage: lua5.1 make-package.lua --corpus <prompted-log> --svof <svof.mpackage> --config <svo-folder>
           --out <SvofBench.mpackage> --salt <file> [--namedb <Database_namedb.db>] [--names <file>]
           [--expect <result.txt>]... [--bundle-version <n>] [--report <file>] [--no-scrub]
       SVOF_SCRUB_SALT=<secret> in the environment can stand in for --salt
]]
local here = arg[0]:match("^(.*)/[^/]*$") or "."

local function usage(msg)
  io.stderr:write("make-package: " .. msg .. "\n" .. USAGE)
  os.exit(2)
end

local VALUED = {corpus = true, svof = true, config = true, out = true, namedb = true, names = true,
  expect = true, ["bundle-version"] = true, report = true, salt = true}
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

local salt
if scrub then
  salt = opts.salt and slurp(opts.salt) or os.getenv("SVOF_SCRUB_SALT")
  if not salt then usage("scrubbing needs a secret salt, from --salt <file> or SVOF_SCRUB_SALT; see README.md") end
  salt = salt:gsub("%s+$", "")
  if #salt < 16 then die("the salt must be at least 16 characters; make one with head -c 32 /dev/urandom | base64") end
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

-- Whether a line's patterns are the ones it should match, or it lost one of them, or only gained some
local function diff(want, got)
  if got == want then return "ok" end
  local have = {}
  for e in got:gmatch("%S+") do have[e] = true end
  for e in want:gmatch("%S+") do
    if not have[e] then return "loses" end
  end
  return "adds"
end

-- Keyed by SHA-256 of the secret salt, so no stand-in or filler can be recomputed from a guess at its word
local keyed
do
  local XOR8, AND8 = {}, {}
  for a = 0, 255 do
    for b = 0, 255 do
      local x, y, bit, aa, bb = 0, 0, 1, a, b
      for _ = 1, 8 do
        local pa, pb = aa % 2, bb % 2
        if pa ~= pb then x = x + bit end
        if pa + pb == 2 then y = y + bit end
        aa, bb, bit = (aa - pa) / 2, (bb - pb) / 2, bit * 2
      end
      XOR8[a * 256 + b], AND8[a * 256 + b] = x, y
    end
  end
  local function op(T, a, b)
    local a0, b0 = a % 256, b % 256
    a, b = (a - a0) / 256, (b - b0) / 256
    local a1, b1 = a % 256, b % 256
    a, b = (a - a1) / 256, (b - b1) / 256
    local a2, b2 = a % 256, b % 256
    return T[a0 * 256 + b0] + T[a1 * 256 + b1] * 256 + T[a2 * 256 + b2] * 65536
      + T[(a - a2) / 256 * 256 + (b - b2) / 256] * 16777216
  end
  local function xor3(a, b, c)
    local a0, b0, c0 = a % 256, b % 256, c % 256
    a, b, c = (a - a0) / 256, (b - b0) / 256, (c - c0) / 256
    local a1, b1, c1 = a % 256, b % 256, c % 256
    a, b, c = (a - a1) / 256, (b - b1) / 256, (c - c1) / 256
    local a2, b2, c2 = a % 256, b % 256, c % 256
    a, b, c = (a - a2) / 256, (b - b2) / 256, (c - c2) / 256
    return XOR8[XOR8[a0 * 256 + b0] * 256 + c0] + XOR8[XOR8[a1 * 256 + b1] * 256 + c1] * 256
      + XOR8[XOR8[a2 * 256 + b2] * 256 + c2] * 65536 + XOR8[XOR8[a * 256 + b] * 256 + c] * 16777216
  end
  local function rotr(x, n)
    local lo = x % 2 ^ n
    return (x - lo) / 2 ^ n + lo * 2 ^ (32 - n)
  end
  local K, n, p = {}, 0, 2
  while n < 64 do
    local prime = true
    for d = 2, math.floor(math.sqrt(p)) do
      if p % d == 0 then
        prime = false
        break
      end
    end
    if prime then
      n = n + 1
      local r = p ^ (1 / 3)
      K[n] = math.floor((r - math.floor(r)) * 2 ^ 32)
    end
    p = p + 1
  end
  local W = {}
  local function compress(H, s, pos)
    for j = 1, 16 do
      local b1, b2, b3, b4 = s:byte(pos + j * 4 - 4, pos + j * 4 - 1)
      W[j] = ((b1 * 256 + b2) * 256 + b3) * 256 + b4
    end
    for j = 17, 64 do
      local w15, w2 = W[j - 15], W[j - 2]
      local s0 = xor3(rotr(w15, 7), rotr(w15, 18), (w15 - w15 % 8) / 8)
      local s1 = xor3(rotr(w2, 17), rotr(w2, 19), (w2 - w2 % 1024) / 1024)
      W[j] = (W[j - 16] + s0 + W[j - 7] + s1) % 4294967296
    end
    local a, b, c, d, e, f, g, h = H[1], H[2], H[3], H[4], H[5], H[6], H[7], H[8]
    for j = 1, 64 do
      local t1 = h + xor3(rotr(e, 6), rotr(e, 11), rotr(e, 25)) + op(XOR8, g, op(AND8, e, op(XOR8, f, g))) + K[j] + W[j]
      local t2 = xor3(rotr(a, 2), rotr(a, 13), rotr(a, 22)) + op(XOR8, op(AND8, a, b), op(AND8, c, op(XOR8, a, b)))
      h, g, f, e, d, c, b, a = g, f, e, (d + t1) % 4294967296, c, b, a, (t1 + t2) % 4294967296
    end
    for j, v in ipairs({a, b, c, d, e, f, g, h}) do H[j] = (H[j] + v) % 4294967296 end
  end
  local function hash(H, msg, before)
    local bits, tail = (before + #msg) * 8, {}
    for k = 8, 1, -1 do
      tail[k] = string.char(bits % 256)
      bits = (bits - bits % 256) / 256
    end
    msg = msg .. "\128" .. ("\0"):rep((55 - before - #msg) % 64) .. table.concat(tail)
    for pos = 1, #msg, 64 do compress(H, msg, pos) end
    return H
  end
  local H0 = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19}
  local mid
  if salt then
    local digest = {}
    for k, v in ipairs(hash({unpack(H0)}, salt, 0)) do
      digest[k] = string.char((v - v % 16777216) / 16777216, (v - v % 65536) / 65536 % 256, (v - v % 256) / 256 % 256, v % 256)
    end
    mid = {unpack(H0)}
    compress(mid, table.concat(digest) .. ("\0"):rep(32), 1)
  end
  -- MRG32k3a, seeded by the hash: once a key is hashed, its draws cost a few multiplications each
  function keyed(key)
    local H = hash({unpack(mid)}, key, 64)
    local m1, m2 = 4294967087, 4294944443
    local a0, a1, a2 = H[1] % m1, H[2] % m1, H[3] % m1
    local b0, b1, b2 = H[4] % m2, H[5] % m2, H[6] % m2
    if a0 + a1 + a2 == 0 then a0 = 1 end
    if b0 + b1 + b2 == 0 then b0 = 1 end
    return function(range)
      local p1 = (1403580 * a1 - 810728 * a0) % m1
      a0, a1, a2 = a1, a2, p1
      local p2 = (527612 * b2 - 1370589 * b0) % m2
      b0, b1, b2 = b1, b2, p2
      return (p1 - p2) % m1 % range + 1
    end
  end
end

-- Letters drawn uniformly in the same case, digits as digits: a word keeps its length and nothing else
local LETTERS, RARE, DIGITS = "abcdefghijklmnopqrstuvwxyz", "jqxz", "0123456789"
local function scramble(s, roll, pool)
  pool = pool or LETTERS
  return (s:gsub("%w", function(c)
    if c:match("%d") then
      local k = roll(10)
      return DIGITS:sub(k, k)
    end
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

local stats = {listed = {}, filled = 0, kept = 0, pinned = 0, renamed = 0, rerolls = 0, trimmed = 0, captured = 0,
  pseudo = 0, capitalised = 0, pseudoWords = 0, svofVocab = 0, gameVocab = 0, redrawn = 0, rare = 0, unplaced = 0,
  standinKeeps = 0, standinRedraws = 0, rounds = 0, categories = {}}
local listed, standins, taken = {}, {}, {}
-- The letters a stand-in keeps are ones svof's patterns match in the word, the rest are drawn
local function drawStandIn(l)
  local s = standins[l]
  for tries = 1, 1000 do
    local f = scramble(l, s.roll):gsub("()%a", function(pos) return s.keep[pos] end)
    -- One- and two-letter words run out of unused stand-ins; a common one is no clue to a name
    if f ~= l and not listed[f] and (not taken[f] or (#l <= 2 and tries > 20 and not listed[l])) then
      taken[f] = true
      s.value = f
      return
    end
  end
  die("no free stand-in for a " .. #l .. "-letter word")
end
local function fakeOf(w)
  local l = w:lower()
  if not standins[l] then
    standins[l] = {roll = keyed("stand-in:" .. l), keep = {}}
    drawStandIn(l)
  end
  return caseLike(w, standins[l].value)
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
-- Words that come out as themselves or a listed name, and filler the gate would strip as svof's marks, are redrawn
local function filler(s, roll, pool)
  local spans = marks(s)
  for _ = 1, 100 do
    local f = scramble(s, roll, pool):gsub("()(%a+)", function(pos, w)
      local was, n = s:sub(pos, pos + #w - 1):lower(), 0
      while listed[w:lower()] or w:lower() == was do
        n = n + 1
        if n > 100 then die("no filler for a " .. #w .. "-letter word") end
        w = scramble(w, roll, pool)
      end
      return w
    end)
    for _, sp in ipairs(spans) do f = f:sub(1, sp[1] - 1) .. s:sub(sp[1], sp[2]) .. f:sub(sp[2] + 1) end
    if #marks(f) == #spans then return f end
  end
  die("no filler that does not read as svof's timings or illusion marks")
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
-- The player's name as svof knows it from GMCP, which is its stand-in once scrubbed
local function meFake(i)
  return me[i] and fakeOf(me[i])
end
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

  -- Every line is filler, unless that changes which of svof's patterns match it. Then svof's own text
  -- is kept: a word stays only if filler in its place loses a match or changes what a pattern captures,
  -- names and capitalised words get the same stand-in in every such line, and anything else is filler
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
    local p, fake = prompt[i2] or false, meFake(i2)
    local w, first, roll = signature(t, p, me[i2]), nil, keyed("line:" .. i2)
    for attempt = 1, 4 do
      local f = filler(t, roll)
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

  -- Which kept lines hold each word, to redo them when its stand-in changes
  local holders = {}
  for _, i2 in ipairs(kept) do
    local seen = {}
    for w in texts[i2]:gmatch("%a+") do
      local l = w:lower()
      if not seen[l] then
        seen[l] = true
        holders[l] = holders[l] or {}
        table.insert(holders[l], i2)
      end
    end
  end

  local REDRAWS = 6
  -- Stand-ins are shared by every line: one that loses a match keeps those letters, one that adds is redrawn
  local function scrubLine(i2)
    local t, p, fake, w, lost = texts[i2], prompt[i2] or false, meFake(i2), want[i2], lostOf[i2]
    local toks, st = tokens(t), {renamed = 0, pseudo = 0, capitalised = 0, captured = 0, pinned = 0, trimmed = 0,
      redrawn = 0, rare = 0, unplaced = 0}
    local stand, fixed, repl, markSpans = {}, {}, {}, marks(t)
    for k, tok in ipairs(toks) do
      local l = tok.w:lower()
      if tok.alpha and (listed[l] or pseudo[l] or (tok.w:match("^%u") and not vocab[l])) then
        stand[k] = l
        repl[k] = fakeOf(tok.w)
      elseif inSpans(markSpans, tok.s, tok.e) then
        fixed[k] = "mark"
      elseif (not tok.alpha or vocab[l]) and inSpans(capSpans[i2], tok.s, tok.e) then
        fixed[k] = "capture"
      end
    end
    local got = signature(splice(t, toks, repl), p, fake)
    if got ~= w then
      -- Each word on its own, beside the player's name: svof matches that name by its stand-in
      local mine, conflicts = {}, {}
      local myName = me[i2] and me[i2]:lower()
      for k, l in pairs(stand) do
        if l == myName then mine[k] = repl[k] end
      end
      local tried = {}
      for k in ipairs(toks) do
        local l = stand[k]
        if l and not tried[l] then
          tried[l] = true
          local only = {}
          for k2, v in pairs(mine) do only[k2] = v end
          for k2, l2 in pairs(stand) do
            if l2 == l then only[k2] = repl[k2] end
          end
          local g = signature(splice(t, toks, only), p, fake)
          local v = diff(w, g)
          if v == "loses" then
            local spans, keep = lostSpans(t, w, g), {}
            for k2 in pairs(only) do
              local tok = toks[k2]
              if stand[k2] == l then
                for pos = tok.s, tok.e do
                  if spans and inSpans(spans, pos, pos) then keep[pos - tok.s + 1] = t:sub(pos, pos):lower() end
                end
              end
            end
            conflicts[l] = next(keep) and {keep = keep} or {redraw = true}
          elseif v == "adds" then
            conflicts[l] = {redraw = true}
          end
        end
      end
      if not next(conflicts) then
        for _, l in pairs(stand) do conflicts[l] = {redraw = true} end
      end
      return nil, conflicts
    end
    for k, l in pairs(stand) do
      if listed[l] then
        st.renamed = st.renamed + 1
      elseif pseudo[l] then
        st.pseudo = st.pseudo + 1
      else
        st.capitalised = st.capitalised + 1
      end
    end
    for _, f in pairs(fixed) do
      if f == "capture" then st.captured = st.captured + 1 end
    end

    local capWant = captureSig(splice(t, toks, repl), lost)
    local function judge(s)
      if #marks(s) ~= #markSpans then return "adds" end
      local v = diff(w, signature(s, p, fake))
      if v == "ok" and captureSig(s, lost) ~= capWant then return "loses" end
      return v
    end
    local roll, draws = keyed("words:" .. i2), {}
    local function draw(k, n)
      local d = draws[k] or {}
      draws[k] = d
      while #d < n do d[#d + 1] = filler(toks[k].w, roll) end
      return d[n]
    end
    local cand, r = {}, {}
    for k in ipairs(toks) do
      if not stand[k] and not fixed[k] then cand[#cand + 1] = k end
    end
    for k, v in pairs(repl) do r[k] = v end
    for _, k in ipairs(cand) do r[k] = draw(k, 1) end
    local result, pinned = splice(t, toks, r), {}
    -- One by one: a filler that only adds matches is drawn again, and a word is kept after two that lose
    if judge(result) ~= "ok" then
      r = {}
      for k, v in pairs(repl) do r[k] = v end
      for _, k in ipairs(cand) do
        local placed, losses = false, 0
        for n = 1, REDRAWS do
          r[k] = draw(k, n)
          local v = judge(splice(t, toks, r))
          if v == "ok" then
            placed = true
            break
          elseif v == "loses" then
            losses = losses + 1
            if losses == 2 then break end
          else
            st.redrawn = st.redrawn + 1
          end
        end
        if not placed and losses == 0 then
          for _ = 1, REDRAWS do
            r[k] = filler(toks[k].w, roll, RARE)
            if judge(splice(t, toks, r)) == "ok" then
              placed = true
              st.rare = st.rare + 1
              break
            end
          end
          if not placed then st.unplaced = st.unplaced + 1 end
        end
        if not placed then
          r[k] = nil
          pinned[k] = true
        end
      end
      result = splice(t, toks, r)
    end
    -- A word kept for a pattern that matches only part of it keeps just that part
    for _, k in ipairs(cand) do
      local tok = toks[k]
      if pinned[k] then
        st.pinned = st.pinned + 1
        if #tok.w > 1 then
          local probe = {}
          for k2, v in pairs(r) do probe[k2] = v end
          probe[k] = draw(k, 1)
          local spans = lostSpans(result, w, signature(splice(t, toks, probe), p, fake))
          if spans and #spans > 0 then
            local one = {[k] = probe[k]}
            keepSpans(t, toks, one, spans)
            if one[k] ~= tok.w and not listed[one[k]:lower()] then
              probe[k] = one[k]
              local s = splice(t, toks, probe)
              if judge(s) == "ok" then
                result, r = s, probe
                st.trimmed = st.trimmed + 1
              end
            end
          end
        end
      end
    end
    return {text = result, st = st}
  end

  local done, todo = {}, kept
  for round = 1, 20 do
    stats.rounds = round
    local conflicts = {}
    for _, i2 in ipairs(todo) do
      local res, c = scrubLine(i2)
      done[i2] = res
      for l, x in pairs(c or {}) do
        local y = conflicts[l] or {keep = {}}
        conflicts[l] = y
        if x.keep then
          for pos, ch in pairs(x.keep) do y.keep[pos] = ch end
        else
          y.redraw = true
        end
      end
    end
    if not next(conflicts) then break end
    if round == 20 then die("stand-ins still change which svof patterns match after 20 rounds") end
    local words, redo = {}, {}
    for l in pairs(conflicts) do words[#words + 1] = l end
    table.sort(words)
    for _, l in ipairs(words) do
      local s, c, n = standins[l], conflicts[l], 0
      for pos, ch in pairs(c.keep) do
        if not s.keep[pos] then stats.standinKeeps = stats.standinKeeps + 1 end
        s.keep[pos] = ch
      end
      for _ in pairs(s.keep) do n = n + 1 end
      if n >= #l then die("svof's patterns need every letter of a " .. #l .. "-letter word that gets a stand-in") end
      local f = s.value:gsub("()%a", function(pos) return s.keep[pos] end)
      if c.redraw or listed[f] or (taken[f] and f ~= s.value) then
        stats.standinRedraws = stats.standinRedraws + 1
        drawStandIn(l)
      else
        taken[f], s.value = true, f
      end
      for _, i2 in ipairs(holders[l] or {}) do redo[i2] = true end
    end
    todo = {}
    for i2 in pairs(redo) do todo[#todo + 1] = i2 end
    table.sort(todo)
  end

  stats.by = {listed = 0, pseudo = 0, other = 0}
  stats.keeping = 0
  for l, s in pairs(standins) do
    local by = listed[l] and "listed" or pseudo[l] and "pseudo" or "other"
    stats.by[by] = stats.by[by] + 1
    if next(s.keep) then stats.keeping = stats.keeping + 1 end
  end
  local reportLines = {}
  for _, i2 in ipairs(kept) do
    local res, lost = done[i2], lostOf[i2]
    stats.kept = stats.kept + 1
    for k, v in pairs(res.st) do stats[k] = stats[k] + v end
    local cat = lost[1] and describe(lost[1]) or "renamed only"
    local c = stats.categories[cat]
    if not c then
      c = {n = 0, example = res.text}
      stats.categories[cat] = c
    end
    c.n = c.n + 1
    reportLines[#reportLines + 1] = i2
    stexts[i2] = res.text
  end
  stats.reportLines = reportLines
end

-- GMCP: keys and vitals stay; a name gets its stand-in, and every other value is filler, except the
-- Char.Status fields svof reads that many players share
local STATUS_KEEP = {class = true, race = true, level = true, gender = true, specialisation = true}
local function scrubGmcp(seg, lineRoll)
  local pkg, body = seg:match("^\255\250\201(%S+)(.-)\255\240$")
  if not pkg or pkg == "Char.Vitals" then return seg end
  local function fill(s)
    return filler(s, lineRoll())
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
    local t, pos, parts, roll = stexts[i], 1, {}, nil
    local function lineRoll()
      roll = roll or keyed("gmcp:" .. i)
      return roll
    end
    for _, s in ipairs(segments(l)) do
      if s.text then
        parts[#parts + 1] = t:sub(pos, pos + #s.text - 1)
        pos = pos + #s.text
      elseif s.proto:sub(2, 2) == "\250" then
        parts[#parts + 1] = scrubGmcp(s.proto, lineRoll)
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
local audit = {names = 0, runs = 0, shared = 0, capitals = 0, standins = 0}
if scrub then
  -- Every line matches the same svof patterns as before
  for i, t in ipairs(stexts) do
    if signature(t, prompt[i] or false, meFake(i)) ~= signature(texts[i], prompt[i] or false, me[i]) then
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

  -- In the lines svof matches, each listed name and capitalised word outside the vocabulary has one stand-in
  local keptSet = {}
  for _, k in ipairs(stats.reportLines) do keptSet[k] = true end
  local survived, seenAs, capitals, mixed = {}, {}, {}, 0
  for i, t in ipairs(texts) do
    local s = stexts[i]
    for pos, w in t:gmatch("()(%a+)") do
      local l, now = w:lower(), s:sub(pos, pos + #w - 1)
      local capital = w:match("^%u") and not vocab[l]
      if capital then
        capitals[l] = true
        if now == w then survived[#survived + 1] = string.format("line %d: a %d-letter word", i, #w) end
      end
      if keptSet[i] and (capital or listed[l]) then
        local as = now:lower()
        if seenAs[l] == nil then
          seenAs[l] = as
          audit.standins = audit.standins + 1
        elseif seenAs[l] ~= as and seenAs[l] ~= false then
          seenAs[l], mixed = false, mixed + 1
        end
      end
    end
  end
  for _ in pairs(capitals) do audit.capitals = audit.capitals + 1 end
  if #survived > 0 then
    die("audit: " .. #survived .. " capitalised words outside the vocabulary survive:\n  " .. table.concat(survived, "\n  ", 1, math.min(#survived, 20)))
  end
  if mixed > 0 then die("audit: " .. mixed .. " words have more than one stand-in in the lines svof matches") end
  local marked = 0
  for i, t in ipairs(texts) do
    if #marks(stexts[i]) ~= #marks(t) then marked = marked + 1 end
  end
  if marked > 0 then die("audit: " .. marked .. " lines gained or lost one of svof's timings or illusion marks") end

  -- The game's fixed messages appear in filler lines too, so a run may stay where svof needs all its words
  local W = 4
  local function unmarked(s)
    for _, m in ipairs(MARKS) do s = s:gsub(m, function(x) return (" "):rep(#x) end) end
    return s
  end
  -- Filler and stand-ins come from the salt, so only a word a line kept can carry another line's text
  local function words(s, raw)
    local ws = {}
    for pos, w in unmarked(s):gmatch("()(%a+)") do
      w = w:lower()
      ws[#ws + 1] = (not raw or raw:sub(pos, pos + #w - 1):lower() == w) and w or "#"
    end
    return ws
  end
  local function each(ws, f)
    for s = 1, #ws - W + 1 do f(table.concat(ws, " ", s, s + W - 1), s) end
  end
  local function needed(i, t, from)
    local p, fake = prompt[i] or false, meFake(i)
    local want, n, roll = signature(t, p, fake), 0, keyed("audit:" .. i)
    local have, spans = {}, {}
    for e in signature(filler(t, roll), p, fake):gmatch("%S+") do have[e] = true end
    for e in want:gmatch("%S+") do
      if not have[e] then
        for _, c in ipairs(entryCaps(t, e)) do spans[#spans + 1] = c end
      end
    end
    for pos, w in unmarked(t):gmatch("()(%a+)") do
      n = n + 1
      if n >= from and n < from + W then
        local changed = vocab[w:lower()] and inSpans(spans, pos, pos + #w - 1)
        for _ = 1, changed and 0 or 4 do
          local f = scramble(w, roll)
          local u = t:sub(1, pos - 1) .. f .. t:sub(pos + #w)
          if f:lower() ~= w:lower() and diff(want, signature(u, p, fake)) == "loses" then
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
  for i, t in ipairs(texts) do
    if not keptSet[i] then each(words(t), function(key) runs[key] = true end) end
  end
  for _ in pairs(runs) do audit.runs = audit.runs + 1 end
  local survivors, shared = {}, {}
  for i, t in ipairs(stexts) do
    each(words(t, texts[i]), function(key, from)
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
  print(string.format("lines: %d filler, %d kept for svof (%d words svof matches on, %d of them cut to the part it matches), %d rerolls",
    stats.filled, stats.kept, stats.pinned, stats.trimmed, stats.rerolls))
  print(string.format("what svof matches and captures: %d words and numbers kept, %d words outside the vocabulary given stand-ins (%d distinct); vocabulary: %d words of svof's text and %d game words in at least %d svof-matched messages",
    stats.captured, stats.pseudo, stats.pseudoWords, stats.svofVocab, stats.gameVocab, stats.gameContexts))
  print(string.format("stand-ins: %d words (%d listed names, %d words svof captures, %d other capitalised words) for %d names, %d captured and %d capitalised words in kept lines; %d keep %d letters svof's patterns match in them, %d drawn again for matching more than the word, in %d rounds",
    stats.by.listed + stats.by.pseudo + stats.by.other, stats.by.listed, stats.by.pseudo, stats.by.other, stats.renamed,
    stats.pseudo, stats.capitalised, stats.keeping, stats.standinKeeps, stats.standinRedraws, stats.rounds))
  print(string.format("filler in kept lines: %d draws matched more than the word and were drawn again, %d words took rare letters instead, %d kept because every filler matched more",
    stats.redrawn, stats.rare, stats.unplaced))
  print(string.format("audit: every line matches the same svof patterns, with svof's marks as they were; 0 of %d listed names left in any case; none of %d capitalised words outside the vocabulary left as it was, and %d names and capitalised words with one stand-in each in the lines svof matches; of %d four-word runs from filler lines, none left except %d in lines that lose a match without every one of those words",
    audit.names, audit.capitals, audit.standins, audit.runs, audit.shared))
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
