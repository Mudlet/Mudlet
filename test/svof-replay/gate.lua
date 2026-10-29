-- Identity gate for a replay buffer: the game's own lines must come through identically, and
-- svof's added output is counted. Its timing annotations vary with wall-clock and TCP chunking,
-- so those are normalised away before hashing.
-- usage: lua5.1 gate.lua <buffer> <plain-log>
local bufferPath, corpusPath = arg[1], arg[2]
if not bufferPath or not corpusPath then
  io.stderr:write("usage: lua5.1 gate.lua <buffer> <plain-log>\n")
  os.exit(2)
end

local function eachLine(path)
  local f = assert(io.open(path, "rb"))
  local data = f:read("*a")
  f:close()
  return data:gmatch("([^\n]*)\n?")
end

local corpus = {}
for l in eachLine(corpusPath) do corpus[(l:gsub("\r$", ""))] = true end

local game, added = {}, {}
for l in eachLine(bufferPath) do
  l = l:gsub("\r$", "")
  if l ~= "" then
    if corpus[l] then
      game[#game + 1] = l
    else
      l = l:gsub(" %(%d+%.%d+s%)", ""):gsub(" %(%d+s%)", ""):gsub(" %(%?s%)", "")
      l = l:gsub("%(%d+%.%d+%)$", "")
      l = l:match("^(%*%*%*%*%*%*%*%*%*%*+) %([%d%.]+s%)$") or l
      if l:match("^%(%d+%.%d+s%)$") then l = "" end
      l = l:gsub("%(svof%): Winnar!", "(svof): You won!")
      if l ~= "" then added[#added + 1] = l end
    end
  end
end

-- Lua 5.1 has no hash function, so md5sum does it
local function md5(label, text)
  io.stdout:flush()
  local p = assert(io.popen("printf '" .. label .. " '; md5sum | cut -c1-8", "w"))
  p:write(text)
  p:close()
end

md5("game_md5", table.concat(game, "\n"))
print("game_lines " .. #game)
print("added_lines " .. #added)
local shapes = {}
for i, l in ipairs(added) do shapes[i] = l:gsub("%d+", "N") end
table.sort(shapes)
md5("added_md5", table.concat(shapes, "\n"))
