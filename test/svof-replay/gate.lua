-- Identity gate for a replay, loaded by bench.lua: the game's own lines must come through, and svof's
-- added output is counted. What svof appends to a line, timings and illusion marks, varies with wall-clock
-- and read chunking, so it is normalised away first.
local gate = {}

local function normal(l)
  l = l:gsub("\r$", ""):gsub(" %(%d+%.%d+s%)", ""):gsub(" %(%d+s%)", ""):gsub(" %(%?s%)", "")
  l = l:gsub("%(%d+%.%d+%)$", "")
  l = l:match("^(%*%*%*%*%*%*%*%*%*%*+) %([%d%.]+s%)$") or l
  if l:match("^%(%d+%.%d+s%)$") then return "" end
  return (l:gsub("%(svof%): Winnar!", "(svof): You won!"):gsub(" %(!?i%)", ""))
end

-- How often each of the corpus's lines occurs, as Mudlet shows it, without its telnet sequences. The log
-- was recorded with svof, so it holds svof's own lines too, and one of those only accounts for one in a run
function gate.plainLines(corpus)
  local count = {}
  for l in corpus:gmatch("([^\n]*)\n?") do
    l = normal(l:gsub("\255\250.-\255\240", ""):gsub("\255[\251-\254].", ""):gsub("\255.", ""))
    if l ~= "" then count[l] = (count[l] or 0) + 1 end
  end
  return count
end

-- Lua 5.1 has no hash function; this one only has to tell two runs apart
local function hash(list)
  local h = 0
  for _, l in ipairs(list) do
    for i = 1, #l, 64 do
      local bytes = {l:byte(i, i + 63)}
      for k = 1, #bytes do h = (h * 31 + bytes[k]) % 4294967296 end
    end
    h = (h * 31 + 10) % 4294967296
  end
  return string.format("%08x", h)
end

-- A buffer's lines as the game's, svof's added ones, and svof's lock warnings
function gate.split(lines, plain)
  local game, added, locks, left = {}, {}, {}, {}
  for _, l in ipairs(lines) do
    l = normal(l)
    if l ~= "" then
      local n = left[l] or plain[l] or 0
      if n > 0 then
        left[l] = n - 1
        game[#game + 1] = l
      -- Repeated at every prompt while svof thinks it is locked, so one run can have hundreds more
      elseif l:find("^%(locks?: ") then
        locks[#locks + 1] = l
      else
        added[#added + 1] = l
      end
    end
  end
  return game, added, locks
end

function gate.check(lines, plain)
  local game, added, locks = gate.split(lines, plain)
  local shapes = {}
  for i, l in ipairs(added) do shapes[i] = l:gsub("%d+", "N") end
  table.sort(shapes)
  return {game_lines = #game, game_hash = hash(game), added_lines = #added, added_hash = hash(shapes),
    lock_lines = #locks}
end

return gate
