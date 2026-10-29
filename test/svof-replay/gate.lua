-- Identity gate for a replay, loaded by bench.lua: the game's own lines must come through, and svof's
-- added output is counted. What svof appends to a line, timings and illusion marks, varies with wall-clock
-- and read chunking, so it is normalised away first.
local gate = {}

-- The corpus's lines as Mudlet shows them, without their telnet sequences
function gate.plainLines(corpus)
  local set = {}
  for l in corpus:gmatch("([^\n]*)\n?") do
    l = l:gsub("\255\250.-\255\240", ""):gsub("\255[\251-\254].", ""):gsub("\255.", ""):gsub("\r$", "")
    set[l] = true
  end
  return set
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

function gate.check(lines, plain)
  local game, added = {}, {}
  for _, l in ipairs(lines) do
    l = l:gsub("\r$", "")
    if l ~= "" then
      if plain[l] then
        game[#game + 1] = l
      else
        l = l:gsub(" %(%d+%.%d+s%)", ""):gsub(" %(%d+s%)", ""):gsub(" %(%?s%)", "")
        l = l:gsub("%(%d+%.%d+%)$", "")
        l = l:match("^(%*%*%*%*%*%*%*%*%*%*+) %([%d%.]+s%)$") or l
        if l:match("^%(%d+%.%d+s%)$") then l = "" end
        l = l:gsub("%(svof%): Winnar!", "(svof): You won!"):gsub(" %(!?i%)", "")
        if l ~= "" and plain[l] then
          game[#game + 1] = l
        -- Repeated at every prompt while svof thinks it is locked, so one run can have hundreds more
        elseif l ~= "" and not l:find("^%(locks?: ") then
          added[#added + 1] = l
        end
      end
    end
  end
  local shapes = {}
  for i, l in ipairs(added) do shapes[i] = l:gsub("%d+", "N") end
  table.sort(shapes)
  return {game_lines = #game, game_hash = hash(game), added_lines = #added, added_hash = hash(shapes)}
end

return gate
