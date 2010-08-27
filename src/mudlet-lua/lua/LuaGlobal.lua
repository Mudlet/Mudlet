----------------------------------------------------------------------------------
--- Mudlet Lua packages loader
----------------------------------------------------------------------------------


if package.loaded["rex_pcre"] then rex = require "rex_pcre" end
if package.loaded["lpeg"] then lpeg = require "lpeg" end
-- TODO this is required by DB.lua, so we might load it all at one place
if package.loaded["luasql.sqlite3"] then require "luasql.sqlite3" end

yajl = require "yajl"
json_to_value = yajl.to_value

gmcp = {}

local PATH_SEP = string.char(getMudletHomeDir():byte()) == "/" and "/" or "\\"
local LUA_DIR = string.format("mudlet-lua%slua%s", PATH_SEP, PATH_SEP)

local packages = {
	"StringUtils.lua",
	"TableUtils.lua",
	"Logging.lua",
	"DebugTools.lua",
	"DB.lua",
	"geyser"..PATH_SEP.."Geyser.lua",
	"geyser"..PATH_SEP.."GeyserGeyser.lua",
	"geyser"..PATH_SEP.."GeyserUtil.lua",
	"geyser"..PATH_SEP.."GeyserColor.lua",
	"geyser"..PATH_SEP.."GeyserSetConstraints.lua",
	"geyser"..PATH_SEP.."GeyserContainer.lua",
	"geyser"..PATH_SEP.."GeyserWindow.lua",
	"geyser"..PATH_SEP.."GeyserLabel.lua",
	"geyser"..PATH_SEP.."GeyserGauge.lua",
	"geyser"..PATH_SEP.."GeyserMiniConsole.lua",
	"geyser"..PATH_SEP.."GeyserReposition.lua",
	-- TODO probably don't need to load this file
	"geyser"..PATH_SEP.."GeyserTests.lua",
	"GUIUtils.lua",
	"Other.lua"
	}

for _, package in ipairs(packages) do
	local result = pcall(dofile, LUA_DIR .. package) or echo("Error attempting to load file: " .. package .. "\n")
end
