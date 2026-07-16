describe("Tests MXP handling", function()

  setup(function()
    -- feedTelnet cannot negotiate MXP here as the test profile's socket is
    -- not in the unconnected state, so force the MXP processor on instead -
    -- that also locks secure mode, letting feedTriggers carry MXP tags
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

end)
