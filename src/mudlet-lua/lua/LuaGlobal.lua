----------------------------------------------------------------------------------
--- Mudlet Lua packages loader
----------------------------------------------------------------------------------


if package.loaded["rex_pcre"] then rex = require "rex_pcre" end
if package.loaded["lpeg"] then lpeg = require "lpeg" end
-- TODO this is required by DB.lua, so we might load it all at one place
--if package.loaded["luasql.sqlite3"] then require "luasql.sqlite3" end

json_to_value = yajl.to_value

gmcp = {}

local packages = {
	"StringUtils.lua",
	"TableUtils.lua",
	"Logging.lua",
	"DebugTools.lua",
	"DB.lua",
	"geyser/Geyser.lua",
	"geyser/GeyserGeyser.lua",
	"geyser/GeyserUtil.lua",
	"geyser/GeyserColor.lua",
	"geyser/GeyserSetConstraints.lua",
	"geyser/GeyserContainer.lua",
	"geyser/GeyserWindow.lua",
	"geyser/GeyserLabel.lua",
	"geyser/GeyserGauge.lua",
	"geyser/GeyserMiniConsole.lua",
	"geyser/GeyserMapper.lua",
	"geyser/GeyserReposition.lua",
	-- TODO probably don't need to load this file
	"geyser/GeyserTests.lua",
	"GUIUtils.lua",
	"Other.lua",
	"GMCP.lua",
	}

for _, package in ipairs(packages) do
	local result = pcall(dofile, "./mudlet-lua/lua/" .. package) or echo("Error attempting to load file: " .. package .. "\n")
end
