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
  -- a merge key whose payload is not a JSON object or array has nothing to merge into
  -- the old table, so take it as is, the same as an unmerged key would
  if _m ~= nil and type(_m) ~= "table" then
    a[key] = _m
    a.__needMerge = nil
    return
  end
  for k, v in pairs(_m) do
    a[key][k] = v;
  end
  a.__needMerge = nil
end


function unzip( what, dest )
  -- an empty dest would gain the separator added below and become the root folder
  if type( dest ) ~= "string" or dest == "" then
    local message = "unzip: a destination folder is needed, got " .. ( dest == "" and "an empty string" or type( dest ) )
    cecho( "<red>ERROR: " .. message .. "\n" )
    return nil, message
  end

  local z, err = zip.open( what )

  if not z then
    cecho("\nerror unpacking: " .. tostring( err ))
    return nil, err
  end

  -- Entry names are joined straight onto dest, so a dest without a trailing
  -- separator would let an entry named "_x" land beside dest instead of in it
  if not dest:find( "[/\\]$" ) then
    dest = dest .. "/"
  end

  local failures = {}
  local function fail( message )
    cecho( "<red>ERROR: " .. message .. "\n" )
    failures[#failures + 1] = message
  end

  -- Two different libraries answer to the global `zip`: Mudlet loads lua-zip
  -- (brimworks) first and only falls back to luazip (Kepler) where brimworks is
  -- missing (see TLuaInterpreter.cpp). brimworks counts entries with
  -- get_num_files() and describes each with stat(i), while Kepler hands back an
  -- iterator from files(), so walk whichever one actually loaded.
  local function entries()
    if type( z.get_num_files ) == "function" then
      local index, count = 0, z:get_num_files()
      return function()
        while index < count do
          index = index + 1
          local info, statErr = z:stat( index )
          if info then
            return { filename = info.name, uncompressed_size = info.size, index = index }
          end
          fail( "can't read archive entry #" .. index .. " (" .. tostring( statErr ) .. ")" )
        end
        return nil
      end
    end
    return z:files()
  end

  -- brimworks rejects io.read's "*a" and wants a byte count, which Kepler also
  -- understands. Kepler returns nil both at the end of an entry and for a failed
  -- read, and never checks the CRC, so the length is checked against the archive's
  -- own record. brimworks reports a CRC mismatch from read() and raises any error
  -- libzip recorded again from close(), where Kepler returns nil and the error.
  local function readEntry( file )
    local handle, openErr = z:open( file.index or file.filename )
    if not handle then
      return nil, openErr
    end
    local chunks = {}
    local ok, readErr = pcall( function()
      while true do
        local chunk, chunkErr = handle:read( 1024 * 1024 )
        if chunkErr then
          error( chunkErr, 0 )
        end
        if not chunk or chunk == "" then
          break
        end
        chunks[#chunks + 1] = chunk
      end
    end )
    local closed, closeResult, closeErr = pcall( handle.close, handle )
    if not ok then
      return nil, readErr
    end
    if not closed then
      return nil, closeResult
    end
    if closeErr then
      return nil, closeErr
    end
    local data = table.concat( chunks )
    if #data ~= file.uncompressed_size then
      return nil, string.format( "truncated, got %d of %d bytes", #data, file.uncompressed_size )
    end
    return data
  end

  -- Resolves "." and ".." within the entry name and returns its components, or
  -- nil if the name is absolute or climbs out of dest. Both separators are split
  -- on because Windows honours either.
  local function entryComponents( name )
    if name:find( "^[/\\]" ) or name:find( "^%a:" ) then
      return nil
    end
    local parts = {}
    for part in name:gmatch( "[^/\\]+" ) do
      if part == ".." then
        if #parts == 0 then
          return nil
        end
        parts[#parts] = nil
      elseif part ~= "." then
        parts[#parts + 1] = part
      end
    end
    return parts
  end

  -- Remembers failures as well as successes, so a folder that cannot be made is
  -- reported once rather than once for every entry inside it
  local madeDirs = {}
  local function makeDirectory( path )
    if madeDirs[path] == nil then
      local made, mkdirErr = true, nil
      -- a symlinked folder already in dest would carry its entries outside dest
      local mode = lfs.symlinkattributes( path, "mode" )
      if mode == "link" then
        made, mkdirErr = nil, "a symbolic link is in the way"
      elseif mode ~= "directory" then
        made, mkdirErr = lfs.mkdir( path )
      end
      madeDirs[path] = made and true or false
      if not made then
        fail( "can't create directory:" .. path .. " (" .. tostring( mkdirErr ) .. ")" )
      end
    end
    return madeDirs[path]
  end

  -- A name beside path for a temporary file. One left by an interrupted run is
  -- cleared, but never one this archive has just extracted.
  local extracted = {}
  local function temporaryName( path, suffix )
    local candidate, attempt = path .. suffix, 1
    while extracted[candidate] do
      attempt = attempt + 1
      candidate = path .. suffix .. attempt
    end
    os.remove( candidate )
    return candidate
  end

  -- Written beside its target and renamed over it, so a failed write leaves a
  -- file already there untouched, and a symlink already there is replaced rather
  -- than written through to wherever it points. Lua buffers writes, so a full
  -- disk usually only shows up at close().
  local function writeFile( path, data )
    local partial = temporaryName( path, ".unzip-partial" )
    local out, openErr = io.open( partial, "wb" )
    if not out then
      return nil, openErr
    end
    local written, writeErr = out:write( data )
    local closed, closeErr = out:close()
    if not ( written and closed ) then
      os.remove( partial )
      return nil, writeErr or closeErr
    end
    local renamed, renameErr = os.rename( partial, path )
    local existing = lfs.symlinkattributes( path, "mode" )
    if not renamed and existing and existing ~= "directory" then
      -- Windows will not rename over an existing file, so it is moved aside and
      -- only deleted once the new one has taken its place
      local backup = temporaryName( path, ".unzip-old" )
      local movedAside, moveErr = os.rename( path, backup )
      if not movedAside then
        os.remove( partial )
        return nil, moveErr
      end
      renamed, renameErr = os.rename( partial, path )
      if renamed then
        os.remove( backup )
      elseif not os.rename( backup, path ) then
        renameErr = tostring( renameErr ) .. ", the file it would have replaced is kept as " .. backup
      end
    end
    if not renamed then
      os.remove( partial )
      return nil, renameErr
    end
    extracted[path] = true
    return true
  end

  for file in entries() do
    local parts = entryComponents( file.filename )
    if not parts then
      fail( "refusing to extract " .. file.filename .. " outside of " .. dest )
    elseif #parts > 0 then
      -- A folder is recognised by its trailing separator rather than by its size,
      -- which it shares with an empty file
      local isDirectory = file.filename:find( "[/\\]$" ) ~= nil
      local folders = isDirectory and #parts or #parts - 1
      local path = dest
      local foldersMade = true
      for i = 1, folders do
        path = path .. parts[i]
        if not makeDirectory( path ) then
          foldersMade = false
          break
        end
        path = path .. "/"
      end

      if not isDirectory then
        path = dest .. table.concat( parts, "/" )
        if not foldersMade then
          fail( "can't write file:" .. path .. " (its folder could not be created)" )
        else
          local data, readErr = readEntry( file )
          if not data then
            fail( "can't read archived file:" .. file.filename .. " (" .. tostring( readErr ) .. ")" )
          else
            local written, writeErr = writeFile( path, data )
            if not written then
              fail( "can't write file:" .. path .. " (" .. tostring( writeErr ) .. ")" )
            end
          end
        end
      end
    end
  end
  z:close()

  if #failures > 0 then
    return nil, table.concat( failures, "\n" )
  end
  return true
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
