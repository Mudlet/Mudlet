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
    -- where unzip() used to put a folder for the archive's top-level empty file
    -- (#10382); every spec extracting unzip-empty.zip can make it on a regression
    local strayFolder = lfs.currentdir() .. "/unzip-spec-empty-top.txt"
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
      else
        os.remove(path)
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
      lfs.rmdir(strayFolder)
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
      lfs.rmdir(strayFolder)
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
      assert.equals(4, echoedCount("refusing to extract"))
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
      assert.is_nil(lfs.attributes(strayFolder))
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

    it("Should report a full disk instead of succeeding silently (#10383)", function()
      if not lfs.link or not lfs.attributes("/dev/full") then
        pending("needs /dev/full to stand in for a full disk")
        return
      end
      -- Lua buffers the write, so this only fails when the file is closed
      assert.is_true(lfs.link("/dev/full", dest .. "config.lua", true))
      local result, err = unzip(archiveDirectory .. "/unzip-empty.zip", dest)
      assert.equals(1, echoedCount("can't write file:" .. dest .. "config.lua ("))
      assert.is_nil(result)
      assert.is_truthy(err:find("can't write file:" .. dest .. "config.lua", 1, true), err)
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
