-- Mudlet Lua packages loader

if package.loaded["rex_pcre2"] then
  rex = require "rex_pcre2"
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

  -- Two different libraries answer to the global `zip`: Mudlet loads lua-zip
  -- (brimworks) first and only falls back to luazip (Kepler) on builds where
  -- brimworks is missing (see TLuaInterpreter.cpp). They enumerate an archive
  -- differently - brimworks counts entries with get_num_files() and describes
  -- each one with stat(i), indexed from 1, while Kepler hands back an iterator
  -- from files() - so walk whichever one actually loaded. Both yield the same
  -- {filename, uncompressed_size} shape the rest of this function expects.
  local function entries()
    if type( z.get_num_files ) == "function" then
      local index, count = 0, z:get_num_files()
      return function()
        index = index + 1
        if index > count then
          return nil
        end
        local info = z:stat( index )
        return { filename = info.name, uncompressed_size = info.size }
      end
    end
    return z:files()
  end

  -- brimworks' entry:read() wants a byte count and rejects io.read's "*a" with
  -- "number expected, got string"; reading in fixed chunks until one comes back
  -- empty is understood by both libraries.
  local function readEntry( filename )
    local _f = z:open( filename )
    if not _f then
      return ""
    end
    local chunks = {}
    while true do
      local chunk = _f:read( 1024 * 1024 )
      if not chunk or chunk == "" then
        break
      end
      chunks[#chunks + 1] = chunk
    end
    _f:close()
    return table.concat( chunks )
  end

  local createdDirs = {}
  for file in entries() do
    local _data = readEntry( file.filename )
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
  end
  z:close()
end



function onConnect()
end

function handleWindowResizeEvent()
end

local packages = {
  "3rdparty/Inspect.lua",

  "StringUtils.lua",
  "TableUtils.lua",

  "DebugTools.lua",
  "DateTime.lua",
  "DB.lua",
  "geyser/Geyser.lua",
  "geyser/GeyserGeyser.lua",
  "geyser/GeyserUtil.lua",
  "geyser/GeyserColor.lua",
  "geyser/GeyserSetConstraints.lua",
  "geyser/GeyserStyleSheet.lua",
  "geyser/GeyserContainer.lua",
  "geyser/GeyserWindow.lua",
  "geyser/GeyserLabel.lua",
  "geyser/GeyserGauge.lua",
  "geyser/GeyserMiniConsole.lua",
  "geyser/GeyserMapper.lua",
  "geyser/GeyserReposition.lua",
  "geyser/GeyserScrollBox.lua",
  "geyser/GeyserHBox.lua",
  "geyser/GeyserVBox.lua",
  "geyser/GeyserUserWindow.lua",
  "geyser/GeyserAdjustableContainer.lua",
  "geyser/GeyserCommandLine.lua",
  "geyser/GeyserTextEdit.lua",
  "geyser/GeyserButton.lua",

  -- TODO probably don't need to load this file
  "geyser/GeyserTests.lua",
  "GUIUtils.lua",
  "Other.lua",
  "GMCP.lua",
  "KeyCodes.lua",
  "CursorShapes.lua",
  "TTSValues.lua",
  "IDManager.lua",
}

-- Set to true (possibly via code in the C++ TLuaInterpreter::loadGlobal()
-- method) to report on the determination of what path to use to load the other
-- Mudlet and Geyser provided Lua files...
debugLoading = debugLoading or false
local sep = package.config:sub(1,1)

if debugLoading then
  echo("Path separator is: '" .. sep .. "'\n\n")

  -- Set via code in C++ TLuaInterpreter::loadGlobal() but fall back to current
  -- directory if nil.
  if luaGlobalPath == nil then
    luaGlobalPath = lfs.currentdir()
    echo("luaGlobalPath was nil so has been defaulted to: \"" .. luaGlobalPath .. "\".\n\n")
  else
    echo("luaGlobalPath has been preset to: \"" .. luaGlobalPath .. "\".\n\n")
  end
  nativeLuaGlobalPath = toNativeSeparators(luaGlobalPath)
  echo("Directory separator conversion gives: \"" .. nativeLuaGlobalPath .. "\".\n\n")
  echo("Current directory is: \"" .. lfs.currentdir() .. "\".\n\n")

  local packagePath, status, result = "", false, ""
  for _, packageName in ipairs(packages) do
    packagePath = nativeLuaGlobalPath .. sep .. toNativeSeparators(packageName)
    echo("Trying to load: \"" .. packagePath .. "\"\n")
    status, result = pcall(dofile, packagePath)
    if (status == false) then
        error("Error attempting to load package("..packageName..") file:\n  " .. result .. ".\n\n")
    end
    echo("Loaded: \"" .. packageName .. "\".\n\n")
  end
else
  -- Set via code in C++ TLuaInterpreter::loadGlobal() but fall back to current
  -- directory if nil.
  luaGlobalPath = luaGlobalPath or lfs.currentdir()
  nativeLuaGlobalPath = toNativeSeparators(luaGlobalPath)

  local packagePath = ""
  for _, packageName in ipairs(packages) do
    packagePath = nativeLuaGlobalPath .. sep .. toNativeSeparators(packageName)
    dofile(packagePath)
  end
end
