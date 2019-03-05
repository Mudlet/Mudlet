local yajl = require 'yajl'
assert(yajl, "yajl is not available (luarocks install lua-yajl)")

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
local file = 'lrelease_output.txt'
local lines = lines_from(file)

local line = 1
local stats = {}

while line <= #lines do
  local currentLine = lines[line]
  local lang = currentLine:match("Updating 'mudlet_([a-z]+_[A-Z]+)%.qm'...")
  line = line + 1
  if lang then
    local translated = 0
    local finished = 0
    local unfinished = 0
    repeat
      currentLine = lines[line]
      translated, finished, unfinished = currentLine:match("Generated (%d+) translation%(s%) %((%d+) finished and (%d+) unfinished%)")
      if translated ~= nil and finished ~= nil and unfinished ~= nil then
        translated = tonumber(translated)
        finished = tonumber(finished)
        unfinished = tonumber(unfinished)
      end
      line = line + 1
    until translated ~= nil
    currentLine = lines[line]
    local untranslated = 0
    if currentLine ~= nil then
      -- The untranslated line will not be present if translation is at 100%!
      untranslated = currentLine:match("Ignored (%d+) untranslated source text%(s%)")
      if untranslated ~= nil then
        untranslated = tonumber(untranslated)
        -- There IS an "Ignored..." line so increment the line counter past it,
        line = line + 1
      else
        -- reset from the nil returned value to 0 (as that how many untranslated
        --  source text there are!)
      end
      -- Otherwise leave it unchanged for the next time around the outer loop
    end

    stats[#stats + 1] = {
      lang = lang,
      translated = translated,
      untranslated = untranslated,
      finished = finished,
      unfinished = unfinished,
      total = translated + untranslated,
      translatedpc = math.floor((100/(translated + untranslated)) * translated)
    }
  end
end

print("lang", "trnsl", "utrnsl", "finish", "unfin", "total", "percentage")
for _, stat in ipairs(stats) do
  print(stat.lang, stat.translated, stat.untranslated, stat.finished, stat.unfinished, stat.total, stat.translatedpc)
end

serialise_stats = {}
for _, stat in ipairs(stats) do
  serialise_stats[stat.lang] = {translated = stat.translated, untranslated = stat.untranslated, total = stat.total, translatedpc = stat.translatedpc}
end

io.output("translation-stats.json")
io.write(yajl.to_string(serialise_stats))
