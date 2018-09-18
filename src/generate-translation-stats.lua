local lunajson = require 'lunajson'
assert(lunajson, "lunajson is not available (luarocks install lunajson)")

-- see if the file exists
function file_exists(file)
  local f = io.open(file, "rb")
  if f then f:close() end
  return f ~= nil
end

-- get all lines from a file, returns an empty
-- list/table if the file does not exist
function lines_from(file)
  if not file_exists(file) then return {} end
  lines = {}
  for line in io.lines(file) do
    lines[#lines + 1] = line
  end
  return lines
end

-- tests the functions above
local file = 'test.txt'
local lines = lines_from(file)

local line = 1
local stats = {}

while line <= #lines do
  local currentLine = lines[line]
  local lang = currentLine:match("mudlet_([a-z]+_[A-Z]+)%.qm")
  line = line + 1
  if lang then
    currentLine = lines[line]
    local translated = tonumber(currentLine:match("Generated (%d+) translation"))
    line = line + 1
    currentLine = lines[line]
    local untranslated = tonumber(currentLine:match("Ignored (%d+) untranslated"))
    line = line + 1
    stats[#stats + 1] = {
      lang = lang,
      translated = translated,
      untranslated = untranslated,
      total = translated + untranslated,
      translatedpc = math.floor((100/(translated + untranslated)) * translated)
    }
  end
end

for _, stat in ipairs(stats) do
  print(stat.lang, stat.translated, stat.untranslated, stat.total, stat.percentage)
end

serialise_stats = {}
for _, stat in ipairs(stats) do
  serialise_stats[stat.lang] = {translated = stat.translated, untranslated = stat.untranslated, total = stat.total, translatedpc = stat.translatedpc}
end

io.output("translation-stats.json")
io.write(lunajson.encode(serialise_stats))