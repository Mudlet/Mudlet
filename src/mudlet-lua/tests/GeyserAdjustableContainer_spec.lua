describe("Tests functionality of Adjustable.Container", function()
  describe("Tests the functionality of Adjustable.Container:addConnectMenu", function()
    local testContainer

    before_each(function()
      -- Create a fresh AdjustableContainer for each test
      testContainer = Adjustable.Container:new({
        name = "testAdjustableContainer",
        x = 0,
        y = 0,
        width = "200px",
        height = "200px",
        autoLoad = false,
        autoSave = false
      })
    end)

    after_each(function()
      -- Clean up the container after each test
      if testContainer then
        testContainer:hide()
      end
    end)

    it("should successfully add connect menu on first call", function()
      -- Should not error on first call
      assert.has_no.errors(function()
        testContainer:addConnectMenu()
      end)

      -- Verify the menu was created
      local connectMenu = testContainer.adjLabel:findMenuElement("Connect To: ")
      assert.is_not_nil(connectMenu)
    end)

    it("should handle multiple calls without error (issue #6318)", function()
      -- This tests the fix for issue #6318 where calling addConnectMenu()
      -- multiple times would cause script errors
      assert.has_no.errors(function()
        testContainer:addConnectMenu()
        testContainer:addConnectMenu()
        testContainer:addConnectMenu()
      end)

      -- Verify the menu still exists and wasn't duplicated
      local connectMenu = testContainer.adjLabel:findMenuElement("Connect To: ")
      assert.is_not_nil(connectMenu)
    end)

    it("should be idempotent - multiple calls should have no observable effect", function()
      -- Call once
      testContainer:addConnectMenu()
      local menuAfterFirstCall = testContainer.rCLabel.MenuItems

      -- Count menu items after first call
      local countAfterFirst = #menuAfterFirstCall

      -- Call again
      testContainer:addConnectMenu()
      local menuAfterSecondCall = testContainer.rCLabel.MenuItems

      -- Count menu items after second call
      local countAfterSecond = #menuAfterSecondCall

      -- The menu should not have grown - it should be the same
      assert.equals(countAfterFirst, countAfterSecond)
    end)

    it("should create the Disconnect menu item", function()
      testContainer:addConnectMenu()

      -- Verify the Disconnect menu was also created
      local disconnectMenu = testContainer.adjLabel:findMenuElement("Disconnect ")
      assert.is_not_nil(disconnectMenu)
    end)

    it("should create the border direction submenu items", function()
      testContainer:addConnectMenu()

      -- Verify the submenu items for connecting to borders were created
      local topMenu = testContainer.adjLabel:findMenuElement("Connect To: .top")
      local bottomMenu = testContainer.adjLabel:findMenuElement("Connect To: .bottom")
      local leftMenu = testContainer.adjLabel:findMenuElement("Connect To: .left")
      local rightMenu = testContainer.adjLabel:findMenuElement("Connect To: .right")

      assert.is_not_nil(topMenu)
      assert.is_not_nil(bottomMenu)
      assert.is_not_nil(leftMenu)
      assert.is_not_nil(rightMenu)
    end)
  end)

  describe("Tests the functionality of Adjustable.Container:new", function()
    it("creates an adjustable container with certain defaults if called with no constraints", function()
      local ac = Adjustable.Container:new({
        name = "testContainer",
        autoLoad = false,
        autoSave = false
      })

      assert.equals("table", type(ac))
      assert.equals("testContainer", ac.name)
      assert.is_not_nil(ac.adjLabel)
      assert.is_not_nil(ac.Inside)
      assert.equals(false, ac.minimized)
      assert.equals(false, ac.locked)
      assert.equals(10, ac.padding)
      assert.equals("standard", ac.lockStyle)

      ac:hide()
    end)
  end)
end)
