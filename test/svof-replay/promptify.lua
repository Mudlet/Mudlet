-- Gives a plain Achaea log the protocol shape svof needs: a GMCP Char.Vitals before every prompt
-- and IAC GA after it. Logs captured with prompts gagged get a synthetic prompt after every N lines,
-- at a sentence end. The stream opens with IAC WILL GMCP and a Char.Status naming the class svof is to
-- play, which need not be the recording character's.
-- usage: lua5.1 promptify.lua <plain-log> <out> <character-name> [every=5] [class=Infernal]
local src, dst, name = arg[1], arg[2], arg[3]
local every = tonumber(arg[4] or "5")
local class = arg[5] or "Infernal"
if not src or not dst or not (name or ""):match("^%a+$") or not every then
  io.stderr:write("usage: lua5.1 promptify.lua <plain-log> <out> <character-name> [every=5] [class=Infernal]\n")
  os.exit(2)
end

local IAC, SB, SE, GA, WILL, GMCP = "\255", "\250", "\240", "\249", "\251", "\201"

local function gmcp(pkg, body)
  return IAC .. SB .. GMCP .. pkg .. " " .. body .. IAC .. SE
end

local STATUS = gmcp("Char.Status", '{ "name": "' .. name .. '", "fullname": "' .. name .. '", "age": "120", "race": "Human", "specialisation": "", "level": "99", "xp": "50%", "xprank": "100", "class": "' .. class .. '", "city": "Mhaldor", "house": "", "order": "", "boundcredits": "0", "unboundcredits": "0", "lessons": "0", "explorerrank": "", "mayancrowns": "0", "boundmayancrowns": "0", "gold": "0", "bank": "0", "unread_news": "0", "unread_msgs": "0", "target": "None", "gender": "male" }')

local function vitals(h, m, bal, eq)
  return gmcp("Char.Vitals", string.format('{ "hp": "%d", "maxhp": "6710", "mp": "%d", "maxmp": "5399", "ep": "28169", "maxep": "28169", "wp": "18809", "maxwp": "18809", "nl": "35", "bal": "%d", "eq": "%d", "vote": "1", "string": "H:%d/6710 M:%d/5399 E:28169/28169 W:18809/18809 NL:35/100 ", "charstats": [ "Bleed: 0", "Rage: 0" ] }', h, m, bal, eq, h, m))
end

local PROMPTS = {"^%(p%) (%d+)h, (%d+)m", "^%d+:%d+:%d+%.%d+ %(OG%) (%d+)h, (%d+)m", "^(%d+)h, (%d+)m"}
local function matchPrompt(line)
  for _, p in ipairs(PROMPTS) do
    local _, e, h, m = line:find(p)
    if e then return e, tonumber(h), tonumber(m) end
  end
end

local f = assert(io.open(src, "rb"))
local data = f:read("*a")
f:close()
local lines, pos = {}, 1
while true do
  local nl = data:find("\n", pos, true)
  if not nl then
    lines[#lines + 1] = data:sub(pos)
    break
  end
  lines[#lines + 1] = data:sub(pos, nl - 1)
  pos = nl + 1
end
if lines[#lines] == "" then lines[#lines] = nil end

local SENTENCE_END = {["."] = true, ["!"] = true, ["?"] = true, ['"'] = true, [")"] = true, ["]"] = true, [":"] = true}
local out = {IAC .. WILL .. GMCP, STATUS, "\n"}
local since, real, synth = 0, 0, 0
for n, line in ipairs(lines) do
  local i = n - 1
  local e, h, m = matchPrompt(line)
  if e then
    local bal = line:sub(e + 1, e + 40):find("x", 1, true) and 1 or 0
    out[#out + 1] = vitals(h, m, bal, 1) .. line .. IAC .. GA .. "\n"
    since = 0
    real = real + 1
  else
    out[#out + 1] = line .. "\n"
    since = since + 1
    local ends = line:gsub("[ \t\r\n\v\f]+$", "")
    if (since >= every and (ends == "" or SENTENCE_END[ends:sub(-1)])) or since >= every * 3 then
      h = 6710 - (i * 37) % 900
      m = 5399 - (i * 53) % 700
      local bal = math.floor(i / 7) % 3 == 0 and 0 or 1
      local flags = bal == 1 and "ex-" or "e-"
      out[#out + 1] = vitals(h, m, bal, 1) .. string.format("%dh, %dm, 28169e, 18809w ", h, m) .. flags .. IAC .. GA .. "\n"
      synth = synth + 1
      since = 0
    end
  end
end
local o = assert(io.open(dst, "wb"))
o:write(table.concat(out))
o:close()
print(string.format("lines in %d real prompts %d synthetic prompts %d lines out %d", #lines, real, synth, #out - 3))
