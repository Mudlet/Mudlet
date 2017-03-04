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

-- override built-in createMiniConsole to allow for multiple calls
do
	local oldcreateMiniConsole = createMiniConsole

	function createMiniConsole(name,x,y,width,height)
		oldcreateMiniConsole(name, 0,0,0,0)
		moveWindow(name,x,y)
		resizeWindow(name,width,height)
	end
end


local packages = {
	"StringUtils.lua",
	"TableUtils.lua",
	-- "Logging.lua", -- never documented and fails to load now
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

-- on windows LuaGlobal gets loaded with the current directory set to mudlet.exe's location
-- on Mac, it's set to LuaGlobals location - or the Applications folder, or something else...
-- So work out where to load Lua files from using some heuristics
-- Addition of "../src/" to front of first path allows things to work when "Shadow Building"
-- option of Qt Creator is used (separates object code from source code in directories
-- beside the latter, allowing parallel builds against different Qt Library versions
-- or {Release|Debug} types).
-- TODO: extend to support common Lua code being placed in system shared directory
-- tree as ought to happen for *nix install builds.
local prefixes = {"../src/mudlet-lua/lua/", "../Resources/mudlet-lua/lua/",
    "mudlet.app/Contents/Resources/mudlet-lua/lua/", "mudlet-lua/lua"}

local prefix
for i = 1, #prefixes do
    if lfs.attributes(prefixes[i]) then
        prefix = prefixes[i]
        break
    end
end

-- For some reason on windows, mudlet-lua/lua/ does not register as a directory with the above strategy.
-- Thus, if chosen we need to append a slash.
if prefix == "mudlet-lua/lua" then
	prefix = prefix .. "/"
end

if not prefix then
        echo("Error locating Lua files from LuaGlobal - we're looking from '"..lfs.currentdir().."'.\n")
	return
end

for _, package in ipairs(packages) do
        local result, msg = pcall(dofile, prefix .. package)
	if not result then echo("Error attempting to load file: " .. package .. ": "..msg.."\n") end
end

