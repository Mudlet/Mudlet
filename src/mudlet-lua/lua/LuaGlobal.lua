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
	-- cecho("\n<blue>unpacking package:<"..what.."< to <"..dest..">\n")
	local z, err = zip.open( what )

	if not z then
		cecho("\nerror unpacking: "..err)
		return
	end

	local createdDirs = {}
	for file in z:files() do
		local _f, err = z:open( file.filename )
		local _data = _f:read("*a")
		local _path = dest .. file.filename
		local _dir = string.split( file.filename, '/' )
		local created = dest;
		for k,v in ipairs( _dir ) do
			if k < # _dir then
				created = created .. '/'.. v;
				if not table.contains( createdDirs, created ) then
					table.insert( createdDirs, created );
					lfs.mkdir( created );
					-- cecho("<red>--> creating dir:" .. created .. "\n");
				end
			elseif file.uncompressed_size == 0 then
				if not table.contains( createdDirs, created ) then
					-- cecho("<red>--> creating dir:" .. file.filename .. "\n")
					table.insert( createdDirs, created );
					lfs.mkdir( file.filename )
				end
			end
		end
		local _path = dest .. file.filename
  		if file.uncompressed_size > 0 then
			local out = io.open( _path, "wb" )
			if out then
				-- cecho("<green>unpacking file:".._path.."\n")
				out:write( _data )
				out:close()
			else
				cecho("<red>ERROR: can't write file:".._path.."\n")
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
	local result, msg = pcall(dofile, "./mudlet-lua/lua/" .. package)
	if not result then echo("Error attempting to load file: " .. package .. ": "..msg.."\n") end
end

