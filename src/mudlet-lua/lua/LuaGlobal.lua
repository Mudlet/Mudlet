----------------------------------------------------------------------------------
--- Mudlet Lua packages loader
----------------------------------------------------------------------------------


if package.loaded["rex_pcre"] then rex = require "rex_pcre" end
if package.loaded["lpeg"] then lpeg = require "lpeg" end
if package.loaded["zip"] then zip = require "zip" end
if package.loaded["lfs"] then lfs = require "lfs" end
if package.loaded["luasql.sqlite3"] then require "luasql.sqlite3" end

json_to_value = yajl.to_value
gmcp = {}

function __gmcp_merge_gmcp_sub_tables( a, key )
	local _m = a.__needMerge;
	for k,v in pairs(_m) do
		a[key][k] = v;
	end
	a.__needMerge = nil
end



--[[The db structure of the map database and some of the Lua code is taken over verbatim from Nick Gammon's open source Mushclient mapper in order to keep maps portable between Mushclient and Mudlet. ]]--

function fixsql (s)  
  if s then
    return "'" .. (string.gsub (s, "'", "''")) .. "'" -- replace single quotes with two lots of single quotes
  else
    return "NULL"
  end 
end

function map_create_tables ()
	db_map:execute([[  
	  PRAGMA foreign_keys = ON;
	  PRAGMA journal_mode = WAL;
	  
	  CREATE TABLE IF NOT EXISTS areas (
		  areaid      INTEGER PRIMARY KEY AUTOINCREMENT,
		  uid         TEXT    NOT NULL,   -- vnum or how the MUD identifies the area
		  name        TEXT,               -- name of area
		  date_added  DATE,               -- date added to database
		  UNIQUE (uid)
		);

	  CREATE TABLE IF NOT EXISTS environments (
		  environmentid INTEGER PRIMARY KEY AUTOINCREMENT,
		  uid           TEXT    NOT NULL,   -- code for the environment
		  name          TEXT,               -- name of environment
		  color         INTEGER,            -- 1-16 are ANSI colours everything else is user defined
		  date_added    DATE,               -- date added to database
		  UNIQUE (uid)
		);
	  CREATE INDEX IF NOT EXISTS name_index ON environments (name);
	   
	  CREATE TABLE IF NOT EXISTS rooms (
		  roomid        INTEGER PRIMARY KEY AUTOINCREMENT,
		  uid           TEXT NOT NULL,   -- vnum or how the MUD identifies the room
		  name          TEXT,            -- name of room
		  area          TEXT,            -- which area
		  terrain       TEXT,            -- eg. road OR water
		  info          TEXT,            -- eg. shop,postoffice
		  notes         TEXT,            -- player notes
		  x             INTEGER,
		  y             INTEGER,
		  z             INTEGER,
		  date_added    DATE,            -- date added to database
		  UNIQUE (uid)
		);
	  CREATE INDEX IF NOT EXISTS info_index ON rooms (info);
	  CREATE INDEX IF NOT EXISTS terrain_index ON rooms (terrain);
	  CREATE INDEX IF NOT EXISTS area_index ON rooms (area);

	  CREATE TABLE IF NOT EXISTS exits (
		  exitid      INTEGER PRIMARY KEY AUTOINCREMENT,
		  dir         TEXT    NOT NULL, -- direction, eg. "n", "s"
		  fromuid     STRING  NOT NULL, -- exit from which room (in rooms table)
		  touid       STRING  NOT NULL, -- exit to which room (in rooms table)
		  date_added  DATE,             -- date added to database
		  FOREIGN KEY(fromuid) REFERENCES rooms(uid)
		);
	  CREATE INDEX IF NOT EXISTS fromuid_index ON exits (fromuid);
	  CREATE INDEX IF NOT EXISTS touid_index   ON exits (touid);
	  
	CREATE TABLE IF NOT EXISTS bookmarks (
      id          INTEGER PRIMARY KEY AUTOINCREMENT,
      uid         TEXT    NOT NULL,   -- vnum of room
      notes       TEXT,               -- user notes
      date_added  DATE,               -- date added to database
      UNIQUE (uid)
    );
    
	CREATE TABLE IF NOT EXISTS terrain (
      id          INTEGER PRIMARY KEY AUTOINCREMENT,
      name        TEXT    NOT NULL,   -- terrain name
      colorRed    INTEGER,            -- RGB 
	  colorGreen  INTEGER,            -- RGB 
	  colorBlue   INTEGER,            -- RGB 
      date_added  DATE,               -- date added to database
      UNIQUE (name)
    );   
  ]])
      
end 

function save_room_to_database (uid, title)
	if not title then title = "" end
	if not uid then return end
	db_map:execute(string.format (
        "INSERT INTO rooms (uid, name, area, date_added) VALUES (%s, %s, '0', DATETIME('NOW'));",
          fixsql(uid), 
          fixsql(title)
    ))
        
	db_map:execute(string.format("INSERT INTO rooms_lookup (uid, name) VALUES (%s, %s);", fixsql  (uid), fixsql(tit)))  
end -- function save_room_to_database
      
function save_room_exits_to_database(uid)  
	local _exits = getRoomExits( uid )
	db_map:exec ("BEGIN TRANSACTION;") 
	for dir,touid in pairs( _exits ) do
      dbcheck (db_map:execute (string.format ([[
        INSERT INTO exits (dir, fromuid, touid, date_added) 
            VALUES (%s, %s, %s, DATETIME('NOW'));
      ]], fixsql(dir),  -- direction (eg. "n")
          fixsql(uid),  -- from current room
          fixsql(touid) -- destination room 
          )))
	end -- for each exit
	local _special_exits = getSpecialExits( uid )
	for dir,touid in pairs( _special_exits ) do
		db_map:execute(string.format([[
			INSERT INTO exits (dir, fromuid, touid, date_added) 
				VALUES (%s, %s, %s, DATETIME('NOW'));
			]], 
			fixsql(dir),  -- direction (eg. "n")
			fixsql(uid),  -- from current room
			fixsql(touid) -- destination room 
          ))
	end
	db:execute ("COMMIT;") 
end

function exportMapToDatabase( dbName )
	env_db_map = assert(luasql.sqlite3())
	db_map = assert(env_db_map:connect(dbName))
	map_create_tables()
	local _rooms = getRooms()
	for k,v in ipairs(_rooms) do
	
	end

end

function importMapFromDatabase( dbName )

end


function unzip( what, dest )   
	local z = zip.open( what )
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
				end
			elseif file.uncompressed_size == 0 then
				if not table.contains( createdDirs, created ) then
					table.insert( createdDirs, created );
					lfs.mkdir( file.filename )
				end
			end
		end
		local _path = dest .. file.filename		
  		if file.uncompressed_size > 0 then
			local out = io.open( _path, "wb" )
			if out then
				out:write( _data )
				out:close()
			else
				cecho("<red>ERROR: Package unzip: Can't write file:".._path.."\n")
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

