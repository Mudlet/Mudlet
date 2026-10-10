describe("Tests LuaGlobal.lua functions", function()

  describe("Tests the functionality of json_to_value", function()
    it("Should decode an object into a table keyed by its member names", function()
      local decoded = json_to_value('{"name":"Vadi","level":42,"online":true}')
      assert.equals("table", type(decoded))
      assert.equals("Vadi", decoded.name)
      assert.equals(42, decoded.level)
      assert.is_true(decoded.online)
    end)

    it("Should decode an array into a 1-based sequence", function()
      local decoded = json_to_value('["a","b","c"]')
      assert.equals(3, #decoded)
      assert.equals("a", decoded[1])
      assert.equals("c", decoded[3])
    end)

    it("Should decode nested objects and arrays", function()
      local decoded = json_to_value('{"room":{"exits":["n","s"],"num":1234}}')
      assert.equals(1234, decoded.room.num)
      assert.equals("n", decoded.room.exits[1])
      assert.equals("s", decoded.room.exits[2])
    end)

    it("Should keep a key whose value is JSON null", function()
      -- GMCP servers use null to say "this field is now unset"; dropping the key
      -- instead would leave the previous value in place after a table update
      local decoded = json_to_value('{"target":null}')
      assert.equals("target", next(decoded))
      assert.is_not_nil(decoded.target)
      assert.equals(1, table.size(decoded))
    end)

    it("Should decode an empty object and an empty array to empty tables", function()
      assert.equals("table", type(json_to_value('{}')))
      assert.equals("table", type(json_to_value('[]')))
      assert.is_nil(next(json_to_value('{}')))
      assert.is_nil(next(json_to_value('[]')))
    end)

    it("Should decode \\u escapes into UTF-8", function()
      local decoded = json_to_value('{"who":"caf\\u00e9"}')
      assert.equals("café", decoded.who)
      -- five bytes for four characters: the escape became UTF-8, not a codepoint
      assert.equals(4, utf8.len(decoded.who))
      assert.equals(5, #decoded.who)
    end)

    it("Should raise on input that is not JSON", function()
      assert.has_error(function() json_to_value('{oops}') end)
    end)
  end)

  describe("Tests the functionality of unzip", function()
    -- unzip() drives whichever library Mudlet preloaded as the global `zip`:
    -- lua-zip (brimworks) on official builds and CI, luazip (Kepler) where it
    -- falls back. Run locally without the brimworks rock, these exercise Kepler.
    local specDirectory = debug.getinfo(1, "S").source:match("^@(.*)[/\\]")
    local archiveDirectory = specDirectory .. "/fixtures/archives"
    local resourcesSource = specDirectory .. "/fixtures/packages/sources/mudlet-spec-resources"
    local root = getMudletHomeDir() .. "/busted-unzip-spec"
    -- nested two deep so that an entry escaping it still lands inside root,
    -- where the teardown finds it
    local dest = root .. "/dest/a/"
    -- an archive's top-level empty file must not become a folder in the working
    -- directory. It is outside root, so it is only removed when a spec made it.
    local strayFolder = lfs.currentdir() .. "/unzip-spec-empty-top.txt"
    local strayFolderExisted
    local echoed
    local originalCecho

    local function removeTree(path)
      local mode = lfs.symlinkattributes(path, "mode")
      if not mode then
        return
      end
      if mode == "directory" then
        local names = {}
        for entry in lfs.dir(path) do
          if entry ~= "." and entry ~= ".." then
            names[#names + 1] = entry
          end
        end
        for _, name in ipairs(names) do
          removeTree(path .. "/" .. name)
        end
        lfs.rmdir(path)
      elseif not os.remove(path) then
        -- Windows reports a symlinked folder as a link, but only rmdir removes it
        lfs.rmdir(path)
      end
    end

    local function contents(path)
      local handle = io.open(path, "rb")
      if not handle then
        return nil
      end
      local data = handle:read("*a")
      handle:close()
      return data
    end

    local function echoedCount(text)
      local count = 0
      for _, line in ipairs(echoed) do
        if line:find(text, 1, true) then
          count = count + 1
        end
      end
      return count
    end

    before_each(function()
      removeTree(root)
      strayFolderExisted = lfs.attributes(strayFolder) ~= nil
      lfs.mkdir(root)
      lfs.mkdir(root .. "/dest")
      lfs.mkdir(root .. "/dest/a")
      -- a leftover tree would let a no-op unzip() pass every check below
      assert.is_nil(lfs.attributes(dest .. "config.lua"))
      echoed = {}
      originalCecho = _G.cecho
      _G.cecho = function(text) echoed[#echoed + 1] = text end
    end)

    after_each(function()
      _G.cecho = originalCecho
      removeTree(root)
      if not strayFolderExisted then
        lfs.rmdir(strayFolder)
      end
    end)

    it("Should report a missing archive rather than raising", function()
      local ok, result, err = pcall(unzip, root .. "/luaGlobalSpecNoSuchArchive.zip", dest)
      assert.is_true(ok, tostring(result))
      assert.equals(1, echoedCount("error unpacking"))
      assert.is_nil(result)
      assert.is_not_nil(err)
    end)

    it("Should extract files and nested folders byte for byte (#10184)", function()
      local ok, result = pcall(unzip, specDirectory .. "/fixtures/packages/mudlet-spec-resources.mpackage", dest)
      assert.is_true(ok, tostring(result))
      for _, name in ipairs({ "config.lua", "mudlet-spec-resources.xml", "resources/spec-note.txt", "resources/nested/spec-nested.txt" }) do
        local expected = contents(resourcesSource .. "/" .. name)
        assert.is_not_nil(expected, name)
        assert.equals(expected, contents(dest .. name), name)
      end
      assert.same({}, echoed)
      assert.is_true(result)
    end)

    it("Should extract into dest when it is given without a trailing separator", function()
      local result = unzip(archiveDirectory .. "/unzip-empty.zip", root .. "/dest/a")
      assert.equals("x\n", contents(dest .. "config.lua"))
      assert.is_nil(lfs.attributes(root .. "/dest/aconfig.lua"))
      assert.is_true(result, table.concat(echoed))
    end)

    it("Should refuse entries whose names lead outside dest (#10381)", function()
      local result, err = unzip(archiveDirectory .. "/unzip-escape.zip", dest)
      assert.is_nil(lfs.attributes(root .. "/dest/escaped.txt"))
      assert.is_nil(lfs.attributes(root .. "/escaped-deep.txt"))
      assert.is_nil(lfs.attributes(dest .. "escaped-absolute.txt"))
      assert.is_nil(lfs.attributes(root .. "/dest/escaped-backslash.txt"))
      assert.is_nil(lfs.attributes(dest .. "..\\escaped-backslash.txt"))
      assert.is_nil(lfs.attributes(dest .. "C:"))
      assert.is_nil(lfs.attributes(dest .. "C:escaped-drive-relative.txt"))
      assert.equals(6, echoedCount("refusing to extract"))
      -- names that resolve to somewhere inside dest are still extracted
      assert.equals("stays inside\n", contents(dest .. "kept.txt"))
      assert.equals("fine\n", contents(dest .. "ok.txt"))
      assert.is_nil(result)
      assert.is_string(err)
    end)

    it("Should extract empty files as files, not skip them or make folders of them (#10382)", function()
      local result = unzip(archiveDirectory .. "/unzip-empty.zip", dest)
      assert.equals("file", lfs.attributes(dest .. "unzip-spec-empty-top.txt", "mode"))
      assert.equals("", contents(dest .. "unzip-spec-empty-top.txt"))
      assert.equals("file", lfs.attributes(dest .. "resources/empty-inside.txt", "mode"))
      assert.equals("", contents(dest .. "resources/empty-inside.txt"))
      assert.equals("note\n", contents(dest .. "resources/note.txt"))
      assert.equals("directory", lfs.attributes(dest .. "resources/nested", "mode"))
      if not strayFolderExisted then
        assert.is_nil(lfs.attributes(strayFolder))
      end
      assert.is_true(result, table.concat(echoed))
    end)

    it("Should report a folder it cannot create once, with the reason (#10383)", function()
      -- a file where the archive wants its resources/ folder makes lfs.mkdir fail
      local blocker = io.open(dest .. "resources", "wb")
      blocker:write("in the way\n")
      blocker:close()
      local result, err = unzip(archiveDirectory .. "/unzip-empty.zip", dest)
      assert.equals(1, echoedCount("can't create directory:" .. dest .. "resources ("))
      assert.equals(2, echoedCount("its folder could not be created"))
      -- entries outside the blocked folder are still extracted
      assert.equals("x\n", contents(dest .. "config.lua"))
      assert.equals("in the way\n", contents(dest .. "resources"))
      assert.is_nil(result)
      assert.is_string(err)
    end)

    it("Should report a full disk and keep the file it would have replaced (#10383)", function()
      local original = io.open(dest .. "config.lua", "wb")
      original:write("original\n")
      original:close()
      -- the file really is opened and written, as a full disk would leave it, and
      -- only fails at close() where Lua's buffered write reaches the disk
      local realOpen = io.open
      io.open = function(path, mode)
        if mode == "wb" and path:find(dest .. "config.lua", 1, true) == 1 then
          local handle = realOpen(path, mode)
          return {
            write = function(self, data) handle:write(data) return self end,
            close = function() handle:close() return nil, "No space left on device" end,
          }
        end
        return realOpen(path, mode)
      end
      local ok, result, err = pcall(unzip, archiveDirectory .. "/unzip-empty.zip", dest)
      io.open = realOpen
      assert.is_true(ok, tostring(result))
      assert.equals(1, echoedCount("can't write file:" .. dest .. "config.lua (No space left on device)"))
      assert.equals("original\n", contents(dest .. "config.lua"))
      local names = {}
      for name in lfs.dir(dest) do
        if name ~= "." and name ~= ".." then
          names[#names + 1] = name
        end
      end
      table.sort(names)
      assert.same({ "config.lua", "resources", "unzip-spec-empty-top.txt" }, names)
      assert.is_nil(result)
      assert.is_truthy(err:find("can't write file:" .. dest .. "config.lua", 1, true), err)
    end)

    it("Should refuse a missing or empty destination rather than extract to the root folder", function()
      -- the archive is never really opened, so a regression cannot write to the root folder
      local realOpen = zip.open
      local opened = 0
      zip.open = function()
        opened = opened + 1
        return nil, "stub archive"
      end
      local results = {}
      for _, badDest in ipairs({ "", false }) do
        results[#results + 1] = { pcall(unzip, root .. "/stub.zip", badDest or nil) }
      end
      zip.open = realOpen
      for _, outcome in ipairs(results) do
        assert.is_true(outcome[1], tostring(outcome[2]))
        assert.is_nil(outcome[2])
        assert.is_truthy(outcome[3]:find("destination folder", 1, true), outcome[3])
      end
      assert.equals(0, opened)
      assert.equals(2, echoedCount("destination folder"))
    end)

    it("Should clear a temporary file an interrupted extraction left behind", function()
      local leftover = io.open(dest .. "config.lua.unzip-partial", "wb")
      leftover:write("half written\n")
      leftover:close()
      local result = unzip(archiveDirectory .. "/unzip-empty.zip", dest)
      assert.is_nil(lfs.attributes(dest .. "config.lua.unzip-partial"))
      assert.equals("x\n", contents(dest .. "config.lua"))
      assert.is_true(result, table.concat(echoed))
    end)

    it("Should replace files from an earlier extraction", function()
      assert.is_true(unzip(archiveDirectory .. "/unzip-empty.zip", dest))
      local changed = io.open(dest .. "config.lua", "wb")
      changed:write("changed since\n")
      changed:close()
      local result = unzip(archiveDirectory .. "/unzip-empty.zip", dest)
      assert.equals("x\n", contents(dest .. "config.lua"))
      assert.equals("note\n", contents(dest .. "resources/note.txt"))
      assert.is_true(result, table.concat(echoed))
    end)

    describe("where a file cannot be renamed over another, as on Windows", function()
      local realRename
      local blockReplacement
      local blockRestore

      before_each(function()
        local original = io.open(dest .. "config.lua", "wb")
        original:write("original\n")
        original:close()
        realRename = os.rename
        blockReplacement = false
        blockRestore = false
        os.rename = function(from, to)
          if lfs.attributes(to) then
            return nil, to .. ": File exists"
          end
          if to == dest .. "config.lua" then
            if blockReplacement and from:find(".unzip-partial", 1, true) then
              return nil, to .. ": Permission denied"
            end
            if blockRestore and from:find(".unzip-old", 1, true) then
              return nil, to .. ": Permission denied"
            end
          end
          return realRename(from, to)
        end
      end)

      after_each(function()
        os.rename = realRename
      end)

      local function names()
        local found = {}
        for name in lfs.dir(dest) do
          if name ~= "." and name ~= ".." then
            found[#found + 1] = name
          end
        end
        table.sort(found)
        return found
      end

      it("Should still replace the file and leave nothing else behind", function()
        local ok, result = pcall(unzip, archiveDirectory .. "/unzip-empty.zip", dest)
        os.rename = realRename
        assert.is_true(ok, tostring(result))
        assert.equals("x\n", contents(dest .. "config.lua"))
        assert.same({ "config.lua", "resources", "unzip-spec-empty-top.txt" }, names())
        assert.is_true(result, table.concat(echoed))
      end)

      it("Should keep the file it could not replace", function()
        blockReplacement = true
        local ok, result, err = pcall(unzip, archiveDirectory .. "/unzip-empty.zip", dest)
        os.rename = realRename
        assert.is_true(ok, tostring(result))
        assert.equals("original\n", contents(dest .. "config.lua"))
        assert.same({ "config.lua", "resources", "unzip-spec-empty-top.txt" }, names())
        assert.equals(1, echoedCount("can't write file:" .. dest .. "config.lua (" .. dest .. "config.lua: Permission denied)"))
        assert.is_nil(result)
        assert.is_string(err)
      end)

      it("Should say where the file it could not replace or put back was kept", function()
        blockReplacement = true
        blockRestore = true
        local ok, result, err = pcall(unzip, archiveDirectory .. "/unzip-empty.zip", dest)
        os.rename = realRename
        assert.is_true(ok, tostring(result))
        assert.equals("original\n", contents(dest .. "config.lua.unzip-old"))
        assert.is_nil(lfs.attributes(dest .. "config.lua"))
        assert.is_nil(lfs.attributes(dest .. "config.lua.unzip-partial"))
        assert.equals(1, echoedCount("kept as " .. dest .. "config.lua.unzip-old"))
        assert.is_nil(result)
        assert.is_string(err)
      end)
    end)

    it("Should not let the temporary file of one entry replace another entry", function()
      local result = unzip(archiveDirectory .. "/unzip-collide.zip", dest)
      assert.equals("an entry of its own\n", contents(dest .. "data.bin.unzip-partial"))
      assert.equals("data\n", contents(dest .. "data.bin"))
      assert.is_true(result, table.concat(echoed))
    end)

    it("Should report an entry whose data does not match its CRC and not write it", function()
      local probe = zip.open(archiveDirectory .. "/unzip-corrupt.zip")
      local checksCrc = type(probe.get_num_files) == "function"
      probe:close()
      if not checksCrc then
        pending("luazip does not check CRCs")
        return
      end
      local ok, result, err = pcall(unzip, archiveDirectory .. "/unzip-corrupt.zip", dest)
      assert.is_true(ok, tostring(result))
      assert.equals(1, echoedCount("can't read archived file:broken.txt ("))
      assert.is_nil(lfs.attributes(dest .. "broken.txt"))
      assert.equals("x\n", contents(dest .. "config.lua"))
      assert.is_nil(result)
      assert.is_string(err)
    end)

    it("Should report an entry that luazip fails to close", function()
      local realOpen = zip.open
      zip.open = function()
        local listed = false
        return {
          files = function()
            return function()
              if listed then
                return nil
              end
              listed = true
              return { filename = "config.lua", uncompressed_size = 2 }
            end
          end,
          open = function()
            local served = false
            return {
              read = function()
                if served then
                  return nil
                end
                served = true
                return "x\n"
              end,
              -- luazip returns its errors rather than raising them
              close = function() return nil, "close failed" end,
            }
          end,
          close = function() end,
        }
      end
      local ok, result, err = pcall(unzip, root .. "/stub.zip", dest)
      zip.open = realOpen
      assert.is_true(ok, tostring(result))
      assert.equals(1, echoedCount("can't read archived file:config.lua (close failed)"))
      assert.is_nil(lfs.attributes(dest .. "config.lua"))
      assert.is_nil(result)
      assert.is_string(err)
    end)

    it("Should report an entry the archive cannot describe and carry on", function()
      local realOpen = zip.open
      zip.open = function()
        return {
          get_num_files = function() return 2 end,
          stat = function(_, index)
            if index == 1 then
              return nil, "Read error"
            end
            return { name = "folder/", size = 0 }
          end,
          close = function() end,
        }
      end
      local ok, result, err = pcall(unzip, root .. "/stub.zip", dest)
      zip.open = realOpen
      assert.is_true(ok, tostring(result))
      assert.equals(1, echoedCount("can't read archive entry #1 (Read error)"))
      assert.equals("directory", lfs.attributes(dest .. "folder", "mode"))
      assert.is_nil(result)
      assert.is_string(err)
    end)

    it("Should not follow a symlinked folder in dest out of it", function()
      lfs.mkdir(root .. "/outside")
      if not lfs.link or not lfs.link(root .. "/outside", dest .. "resources", true) then
        pending("needs symbolic links")
        return
      end
      local result, err = unzip(archiveDirectory .. "/unzip-empty.zip", dest)
      assert.is_nil(lfs.attributes(root .. "/outside/note.txt"))
      assert.is_nil(lfs.attributes(root .. "/outside/empty-inside.txt"))
      assert.is_nil(lfs.attributes(root .. "/outside/nested"))
      assert.equals(1, echoedCount("can't create directory:" .. dest .. "resources (a symbolic link is in the way)"))
      assert.equals("x\n", contents(dest .. "config.lua"))
      assert.is_nil(result)
      assert.is_string(err)
    end)

    it("Should replace a symlinked file in dest rather than write through it", function()
      local outside = io.open(root .. "/outside.txt", "wb")
      outside:write("untouched\n")
      outside:close()
      if not lfs.link or not lfs.link(root .. "/outside.txt", dest .. "config.lua", true) then
        pending("needs symbolic links")
        return
      end
      local result = unzip(archiveDirectory .. "/unzip-empty.zip", dest)
      assert.equals("untouched\n", contents(root .. "/outside.txt"))
      assert.equals("file", lfs.symlinkattributes(dest .. "config.lua", "mode"))
      assert.equals("x\n", contents(dest .. "config.lua"))
      assert.is_true(result, table.concat(echoed))
    end)
  end)

  describe("Tests the Lua module search path", function()
    -- toNativeSeparators() was once defined in LuaGlobal.lua, which loads long
    -- after these search paths are built, so the assignment that uses it failed
    -- and a module dropped into the profile directory was never found
    it("Should carry the profile's own directory on package.path (#4051)", function()
      -- compared with the separators flattened because package.path is built by
      -- calling toNativeSeparators itself, so using it here too would let one
      -- broken conversion agree with itself
      local wanted = (getMudletHomeDir() .. "/?.lua"):gsub("\\", "/")
      local havePath = package.path:gsub("\\", "/")
      assert.is_truthy(havePath:find(wanted, 1, true), package.path)
    end)
  end)

  describe("Tests the globals LuaGlobal.lua seeds", function()
    it("Should still have gmcp and mssp as tables once every package has loaded", function()
      -- the protocol handlers index straight into these, so a package loaded
      -- after this line shadowing either name breaks GMCP for the whole profile
      assert.equals("table", type(gmcp))
      assert.equals("table", type(mssp))
    end)

    it("Should record where the Lua library was loaded from", function()
      -- the packages list is dofile'd off this path, so a wrong value here means
      -- the profile ran a different copy of mudlet-lua than the one it reports
      assert.is_string(luaGlobalPath)
      assert.is_truthy(lfs.attributes(luaGlobalPath .. "/LuaGlobal.lua"))
      assert.is_truthy(lfs.attributes(nativeLuaGlobalPath .. "/LuaGlobal.lua"))
    end)
  end)
end)

describe("lfs directory objects", function()
  it("raise an error when close is called without the object", function()
    local _, dir = lfs.dir(getMudletHomeDir())
    assert.has_error(function() dir.close() end)
    assert.has_error(function() dir.close(io.stdout) end)
    dir:close()
  end)

  it("still close and stop iterating", function()
    local _, dir = lfs.dir(getMudletHomeDir())
    assert.truthy(dir:next())
    dir:close()
    assert.has_error(function() dir:next() end)
  end)
end)
