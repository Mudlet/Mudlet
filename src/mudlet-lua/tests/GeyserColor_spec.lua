describe("Tests functionality of Geyser.Color", function()
  describe("Geyser.Color.find_color_name", function()
    it("finds a colour however it is capitalised", function()
      assert.are.equal("white", Geyser.Color.find_color_name("white"))
      assert.are.equal("white", Geyser.Color.find_color_name("WHITE"))
      assert.are.equal("white", Geyser.Color.find_color_name("White"))
    end)

    -- underscores come out of what is asked for but not out of what color_table
    -- holds, so an underscored name only finds an unspaced entry
    it("strips underscores from the name it is given", function()
      assert.are.equal("LightBlue", Geyser.Color.find_color_name("light_blue"))
      assert.are.equal("LightBlue", Geyser.Color.find_color_name("lightblue"))
    end)

    it("answers false for anything that is not a colour name", function()
      assert.is_false(Geyser.Color.find_color_name("no_such_colour_at_all"))
      assert.is_false(Geyser.Color.find_color_name(nil))
      assert.is_false(Geyser.Color.find_color_name(42))
      assert.is_false(Geyser.Color.find_color_name({}))
    end)

    -- the lookup is indexed rather than scanned, so an index built before a
    -- script added its own colour has to notice it
    it("finds a colour added to color_table after the first lookup", function()
      assert.is_false(Geyser.Color.find_color_name("gcsAddedColour"))
      finally(function() color_table.gcsAddedColour = nil end)
      color_table.gcsAddedColour = {12, 34, 56}
      assert.are.equal("gcsAddedColour", Geyser.Color.find_color_name("gcsaddedcolour"))
      assert.are.same({12, 34, 56, 255}, {Geyser.Color.parse("gcsAddedColour")})
    end)

    it("stops finding a colour taken back out of color_table", function()
      local restore = color_table.white
      finally(function() color_table.white = restore end)
      assert.are.equal("white", Geyser.Color.find_color_name("white"))
      color_table.white = nil
      assert.is_false(Geyser.Color.find_color_name("white"))
      -- and putting it back finds it again, so nothing later in the suite
      -- inherits a lookup that has lost white
      color_table.white = restore
      assert.are.equal("white", Geyser.Color.find_color_name("white"))
    end)
  end)

  describe("Geyser.Color.parse", function()
    it("reads a named colour", function()
      assert.are.same({255, 255, 255, 255}, {Geyser.Color.parse("white")})
    end)

    it("reads hex, hecho, 0x and decho forms", function()
      assert.are.same({170, 0, 255, 255}, {Geyser.Color.parse("#AA00FF")})
      assert.are.same({170, 0, 255, 255}, {Geyser.Color.parse("|cAA00FF")})
      assert.are.same({170, 0, 255, 255}, {Geyser.Color.parse("0xAA00FF")})
      assert.are.same({190, 0, 255, 255}, {Geyser.Color.parse("<190,0,255>")})
      assert.are.same({190, 0, 255, 128}, {Geyser.Color.parse("<190,0,255,128>")})
    end)

    it("reads discrete components and a table", function()
      assert.are.same({1, 2, 3, 255}, {Geyser.Color.parse(1, 2, 3)})
      assert.are.same({1, 2, 3, 4}, {Geyser.Color.parse(1, 2, 3, 4)})
      assert.are.same({1, 2, 3, 4}, {Geyser.Color.parse({r = 1, g = 2, b = 3, a = 4})})
    end)

    -- only the names are looked up, never the components behind them: Mudlet
    -- rewrites color_table's entries in place when the ANSI palette changes
    it("gives the current value of a colour whose entry changed", function()
      local restore = color_table.white
      finally(function() color_table.white = restore end)
      assert.are.same({255, 255, 255, 255}, {Geyser.Color.parse("white")})
      color_table.white = {1, 2, 3}
      assert.are.same({1, 2, 3, 255}, {Geyser.Color.parse("white")})
    end)

    it("gives nothing for a colour it cannot read", function()
      assert.is_nil(Geyser.Color.parse("no_such_colour_at_all"))
      assert.is_nil(Geyser.Color.parse(nil))
    end)
  end)

  describe("Geyser.Color formatting", function()
    it("formats a named colour every way round", function()
      assert.are.equal("#ffffff", Geyser.Color.hex("white"))
      assert.are.equal("#ffffffff", Geyser.Color.hexa("white"))
      assert.are.equal("|cffffff", Geyser.Color.hhex("white"))
      assert.are.equal("<255,255,255>", Geyser.Color.hdec("white"))
      assert.are.equal("<255,255,255,255>", Geyser.Color.hdeca("white"))
    end)
  end)
end)
