describe("Tests MXP handling", function()

  setup(function()
    -- these specs carry MXP tags in through feedTriggers, so the processor is
    -- forced on: that is what locks secure mode, which negotiating MXP with a
    -- server would not do
    setConfig("specialForceMXPProcessorOn", true)
  end)

  teardown(function()
    -- turn the MXP processor back off so later specs are not affected
    setConfig("specialForceMXPProcessorOn", false)
  end)

  describe("Tests custom element events populate the mxp table", function()
    it("should expose positional values under declared (ATT) attribute names", function()
      feedTriggers([[<!ELEMENT RMob FLAG="RoomMob" ATT="Name">]] .. "\n")
      feedTriggers([[<RMob "Urthguk">Urthguk, a hulking half-ogre</RMob>]] .. "\n")

      assert.is_table(mxp)
      assert.is_table(mxp.rmob)
      -- declared attribute resolves the positional token, value case intact
      assert.are.equal("Urthguk", mxp.rmob.name)
      -- the legacy lowercased token key remains for older scripts
      assert.is_not_nil(mxp.rmob.urthguk)
      assert.is_nil(mxp.rmob.Urthguk)
    end)

    it("should lowercase named attribute keys", function()
      feedTriggers([[<!ELEMENT RExit FLAG="RoomExit">]] .. "\n")
      feedTriggers([[<RExit Dir="North">north</RExit>]] .. "\n")

      assert.is_table(mxp)
      assert.is_table(mxp.rexit)
      assert.are.equal("North", mxp.rexit.dir)
      assert.is_nil(mxp.rexit.Dir)
    end)

    it("should keep positional token keys lowercased without a declared attribute", function()
      feedTriggers([[<!ELEMENT RItem FLAG="RoomItem">]] .. "\n")
      feedTriggers([[<RItem "Sword">a gleaming sword</RItem>]] .. "\n")

      assert.is_table(mxp)
      assert.is_table(mxp.ritem)
      assert.is_not_nil(mxp.ritem.sword)
      assert.is_nil(mxp.ritem.Sword)
    end)
  end)

  describe("Tests the text an MXP line is displayed as", function()
    local encodingFile = getMudletHomeDir() .. "/encoding"
    local hadEncodingFile, originalEncoding

    setup(function()
      -- the payloads below carry their non-ASCII characters as the multi-byte
      -- sequences a game sends; under another encoding the decoder reassembles
      -- them into something else and the assertions stop meaning anything.
      -- setServerEncoding() writes the profile's "encoding" file, and a profile
      -- that never had one must not be left with one.
      hadEncodingFile = lfs.attributes(encodingFile, "mode") ~= nil
      originalEncoding = getServerEncoding()
      setServerEncoding("UTF-8")
    end)

    teardown(function()
      setServerEncoding(originalEncoding)
      if not hadEncodingFile then
        os.remove(encodingFile)
      end
    end)

    -- The payloads are what a game sends, ESC[#z mode switches and all, but
    -- those bytes are inert here: TBuffer only acts on them for data flagged as
    -- coming from a server (TBuffer.cpp's isFromServer gate), which feedTriggers
    -- is not. Secure mode comes from the forced processor this file turns on.
    -- feedTelnet would flag the data and run the mode switches, but its ESC[1z
    -- also makes cTelnet auto-enable MXP, and only a new connection clears that
    -- flag - the teardown's setConfig cannot, so every later spec file would
    -- inherit an enabled MXP processor. The tag and entity handling under test
    -- is the same either way.
    local function displayedLines(data)
      local mark = getLastLineNumber("main")
      feedTriggers(data .. "\n")
      return getLines("main", mark, getLastLineNumber("main") + 1)
    end

    -- a whole line, not a substring: an entity that resolved to the wrong thing
    -- would still be a substring of the line it landed on
    local function assertLineShown(data, expected)
      local lines = displayedLines(data)
      local shown = false
      for _, line in ipairs(lines) do
        shown = shown or line == expected
      end
      assert.is_true(shown, ("no line reads %q, the console shows:\n%s"):format(expected, table.concat(lines, "\n")))
    end

    it("takes the tags out of an MXP line", function()
      assertLineShown("\27[1z<B>Greetings < hunters & sorcerers</B>\27[7z", "Greetings < hunters & sorcerers")
    end)

    -- an unescaped & running into a non-ASCII character is not an entity, so
    -- the raw bytes have to be passed through for the charset decoder to
    -- reassemble them (follow-up to #9439)
    it("keeps the non-ASCII bytes around a malformed entity", function()
      assertLineShown("\27[1zKäse&Brötchen and &Ф too", "Käse&Brötchen and &Ф too")
    end)

    -- a custom entity whose value is not Latin1 (#9439)
    it("resolves a custom entity to its non-ASCII value", function()
      assertLineShown("\27[1z<!ENTITY storm \"Гроза\">The &storm; rages", "The Гроза rages")
    end)
  end)

end)
