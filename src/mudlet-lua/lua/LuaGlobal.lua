----------------------------------------------------------------------------------
--- Mudlet Lua packages loader
----------------------------------------------------------------------------------


if package.loaded["rex_pcre"] then rex = require "rex_pcre" end
if package.loaded["lpeg"] then lpeg = require "lpeg" end
if package.loaded["zip"] then zip = require "zip" end
if package.loaded["lfs"] then lfs = require "lfs" end

-- TODO this is required by DB.lua, so we might load it all at one place
--if package.loaded["luasql.sqlite3"] then require "luasql.sqlite3" end

json_to_value = yajl.to_value
gmcp = {}

function __gmcp_merge_gmcp_sub_tables( a, key )
	local _m = a.__needMerge;
	for k,v in pairs(_m) do
		a[key][k] = v;
	end
	a.__needMerge = nil
end


function unzip( what, dest )    
	local z = zip.open( what )
	local _first = true
	
	for file in z:files() do
		if _first then
			local _dir
			for _dir in string.gmatch(file.filename, "(%w+)/") do
       			lfs.mkdir( dest .. _dir )
			end
			_first = false	
		end
		
		local _f, err = z:open( file.filename )
		local _data = _f:read("*a")
		local _path = dest .. file.filename
	
  		if file.compressed_size == 0 then
			lfs.mkdir( _path )
		else
			local out = io.open( _path, "wb" )
			if out then
				out:write( _data )
				out:close()
			end
		end
		_f:close();
	end
	z:close()
end



function onConnect()
end

function handleWindowResizeEvent()
end


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
	"geyser/GeyserHBox.lua",
	"geyser/GeyserVBox.lua",
	-- TODO probably don't need to load this file
	"geyser/GeyserTests.lua",
	"GUIUtils.lua",
	"Other.lua",
	"GMCP.lua",
	}

for _, package in ipairs(packages) do
	local result = pcall(dofile, "./mudlet-lua/lua/" .. package) or echo("Error attempting to load file: " .. package .. "\n")
end

