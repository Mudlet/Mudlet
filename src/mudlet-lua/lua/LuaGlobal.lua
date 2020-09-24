-- Mudlet Lua packages loader

-- Set to true (possibly via code in the C++ TLuaInterpreter::loadGlobal()
-- method) to report on the determination of what path to use to load the other
-- Mudlet and Geyser provided Lua files...
debugLoading = debugLoading or false

-- Set via code in C++ TLuaInterpreter::loadGlobal() but fall back to current
-- directory if nil.
if luaGlobalPath == nil then
  luaGlobalPath = lfs.currentdir() .. package.config:sub(1,1) .. "LuaGlobal.lua"
  if debugLoading then
    echo([[luaGlobalPath was nil so has been defaulted to: "]] .. luaGlobalPath .. [[".

]])
  end
elseif debugLoading then
  echo([[luaGlobalPath has been preset to: "]] .. luaGlobalPath .. [[".

]])
end

if package.loaded["rex_pcre"] then
  rex = require "rex_pcre"
end
if package.loaded["lpeg"] then
  lpeg = require "lpeg"
end
if package.loaded["zip"] then
  zip = require "zip"
end
if package.loaded["lfs"] then
  lfs = require "lfs"
end

-- TODO this is required by DB.lua, so we might load it all at one place
--if package.loaded["luasql.sqlite3"] then require "luasql.sqlite3" end

json_to_value = yajl.to_value
gmcp = {}
mssp = {}

function __gmcp_merge_gmcp_sub_tables( a, key )
  local _m = a.__needMerge;
  for k, v in pairs(_m) do
    a[key][k] = v;
  end
  a.__needMerge = nil
end


function unzip( what, dest )
  -- cecho("\n<blue>unpacking package:<"..what.."< to <"..dest..">\n")
  local z, err = zip.open( what )

  if not z then
    cecho("\nerror unpacking: " .. err)
    return
  end

  local createdDirs = {}
  for file in z:files() do
    local _f, err = z:open( file.filename )
    local _data = _f:read("*a")
    local _path = dest .. file.filename
    local _dir = string.split( file.filename, '/' )
    local created = dest;
    for k, v in ipairs( _dir ) do
      if k < #_dir then
        created = created .. '/' .. v;
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
        cecho("<red>ERROR: can't write file:" .. _path .. "\n")
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
  "geyser/GeyserUserWindow.lua",
  "geyser/GeyserAdjustableContainer.lua",
  "geyser/GeyserCommandLine.lua",

  -- TODO probably don't need to load this file
  "geyser/GeyserTests.lua",
  "GUIUtils.lua",
  "Other.lua",
  "GMCP.lua",
  "KeyCodes.lua",
  "CursorShapes.lua",
  "TTSValues.lua"
}

if debugLoading then
   echo("Path separator is: '" .. package.config:sub(1,1) .. "'\n\n")
end

nativeLuaGlobalPath = toNativeSeparators(luaGlobalPath)

if debugLoading then
  echo([[Directory separator conversion gives: "]] .. nativeLuaGlobalPath .. [[".

Current directory is: "]] .. lfs.currentdir() .. [[".

]])
end

for _, packageName in ipairs(packages) do
  local packagePath = nativeLuaGlobalPath .. package.config:sub(1,1) .. toNativeSeparators(packageName)
  if debugLoading then
    echo([[Trying to load: "]] .. packagePath .. [["
]])
  end
  local result, msg = pcall(dofile, packagePath)
  if debugLoading then
    if result then
      echo([[Loaded: "]] .. packageName .. [[".

]])
    else
      echo([[Error attempting to load file:
  ]] .. msg .. [[.

]])
    end
  end
  assert(result, msg)
end
