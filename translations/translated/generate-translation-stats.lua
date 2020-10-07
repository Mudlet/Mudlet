-- Convert output from runs of Qt lrelease for each language into statistics
--[[
 ############################################################################
 #    Copyright (C) 2018-2019 by Vadim Peretokin - vperetokin@gmail.com     #
 #    Copyright (C) 2018 by Florian Scheel - keneanung@googlemail.com       #
 #    Copyright (C) 2019-2020 by Stephen Lyons - slysven@virginmedia.com    #
 #                                                                          #
 #    This program is free software; you can redistribute it and/or modify  #
 #    it under the terms of the GNU General Public License as published by  #
 #    the Free Software Foundation; either version 2 of the License, or     #
 #    (at your option) any later version.                                   #
 #                                                                          #
 #    This program is distributed in the hope that it will be useful,       #
 #    but WITHOUT ANY WARRANTY; without even the implied warranty of        #
 #    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
 #    GNU General Public License for more details.                          #
 #                                                                          #
 #    You should have received a copy of the GNU General Public License     #
 #    along with this program; if not, write to the                         #
 #    Free Software Foundation, Inc.,                                       #
 #    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
 ############################################################################
]]

local status, result = pcall(require, 'yajl')
if not status then
  print("warning: lua-yajl not available - translation statistics in settings won't be shown.")
  io.output("translation-stats.json")
  io.write("{}")

  return
end

local yajl = result

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
  local lang = currentLine:match("Updating '.*mudlet_([a-z]+_[A-Z]+)%.qm'...")
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
      untranslated = untranslated or 0,
      finished = finished,
      unfinished = unfinished,
      total = translated + (untranslated or 0),
      translatedpc = math.floor((100 * translated)/(translated + (untranslated or 0)))
    }
  end
end

print()
print("We have statistics for " .. #stats .. " languages:")
print()
print("   lang_CNTRY    trnsl  utrnsl  finish  unfin  total  done")
for _, stat in ipairs(stats) do
  local star = ' '
  if stat.translatedpc > 94 then
    star = '*'
  end
  print(string.format("%1s    %-10s  %5d   %5d   %5d  %5d  %5d  %3d%%", star, stat.lang, stat.translated, stat.untranslated, stat.finished, stat.unfinished, stat.total, stat.translatedpc))
end
print()

serialise_stats = {}
for _, stat in ipairs(stats) do
  serialise_stats[stat.lang] = {translated = stat.translated, untranslated = stat.untranslated, total = stat.total, translatedpc = stat.translatedpc}
end

io.output("translation-stats.json")
io.write(yajl.to_string(serialise_stats))
