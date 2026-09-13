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
    it("Should report a missing archive rather than raising", function()
      local home = getMudletHomeDir()
      local echoed
      local originalCecho = _G.cecho
      _G.cecho = function(text) echoed = text end
      finally(function() _G.cecho = originalCecho end)
      local ok, err = pcall(unzip, home .. "/luaGlobalSpecNoSuchArchive.zip", home .. "/")
      assert.is_true(ok, tostring(err))
      assert.is_string(echoed)
      assert.is_truthy(echoed:find("error unpacking", 1, true))
    end)

    -- TLuaInterpreter loads the brimworks lua-zip rock as 'zip' and only falls
    -- back to luazip, but unzip() is written against luazip's z:files() iterator
    pending("unzip extracts an archive's files and directories - against the lua-zip binding it raises \"attempt to call method 'files'\" - issue #10184")
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
