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

  -- Geometry, visibility and title readback, asserted on the widgets the
  -- container builds rather than on its bookkeeping alone.
  describe("Adjustable.Container widget state", function()
    local container

    local function geometry(name)
      local x, y, width, height = getWindowGeometry(name)
      return {x = x, y = y, width = width, height = height}
    end

    before_each(function()
      container = Adjustable.Container:new({
        name = "gasContainer",
        x = 20,
        y = 30,
        width = 200,
        height = 200,
        autoLoad = false,
        autoSave = false,
      })
    end)

    after_each(function()
      if container and Geyser.windowList.gasContainer == container then
        container:delete()
      end
      container = nil
    end)

    it("puts its backdrop label over the container's geometry", function()
      assert.are.equal("label", windowType("gasContaineradjLabel"))
      assert.are.same({x = 20, y = 30, width = 200, height = 200}, geometry("gasContaineradjLabel"))
      assert.is_true(windowVisible("gasContaineradjLabel"))
    end)

    it("drags its labels along when the container moves and resizes", function()
      container:move(60, 70)
      container:resize(100, 120)
      assert.are.same({x = 60, y = 70, width = 100, height = 120}, geometry("gasContaineradjLabel"))
    end)

    it("hides and shows every widget it owns", function()
      container:hide()
      assert.is_false(windowVisible("gasContaineradjLabel"))
      container:show()
      assert.is_true(windowVisible("gasContaineradjLabel"))
    end)

    it("writes the title onto the backdrop label", function()
      container:setTitle("My Title", "red", "c")
      local text = getLabelText("gasContaineradjLabel")
      assert.is_truthy(text:find("My Title", 1, true))
      assert.is_truthy(text:find("color: #ff0000", 1, true))
      assert.is_truthy(text:find('align="center"', 1, true))
      assert.are.equal("My Title", container.titleText)
    end)

    it("titles itself after its name to begin with", function()
      assert.are.equal("gasContainer - Adjustable Container", container.titleText)
      assert.is_truthy(getLabelText("gasContaineradjLabel"):find("gasContainer - Adjustable Container", 1, true))
    end)

    it("stops drawing the title while the container is locked", function()
      container:setTitle("before lock")
      assert.is_truthy(getLabelText("gasContaineradjLabel"):find("before lock", 1, true))
      -- the standard lock style clears the title bar so the container reads as
      -- locked down
      container:lockContainer()
      assert.is_true(container.locked)
      assert.is_nil(getLabelText("gasContaineradjLabel"):find("before lock", 1, true))
      container:setTitle("after lock")
      assert.are.equal("after lock", container.titleText)
      assert.is_nil(getLabelText("gasContaineradjLabel"):find("after lock", 1, true))
      -- unlocking redraws the title that was stored while locked
      container:unlockContainer()
      assert.is_false(container.locked)
      assert.is_truthy(getLabelText("gasContaineradjLabel"):find("after lock", 1, true))
    end)

    it("shrinks to the title bar when minimized and grows back when restored", function()
      container:minimize()
      assert.is_true(container.minimized)
      local minimized = geometry("gasContaineradjLabel")
      assert.are.equal(200, minimized.width)
      assert.is_true(minimized.height < 200)
      assert.are.equal(container.buttonsize + 10, minimized.height)
      container:restore()
      assert.is_false(container.minimized)
      assert.are.same({x = 20, y = 30, width = 200, height = 200}, geometry("gasContaineradjLabel"))
    end)

    it("deletes all of its widgets", function()
      container:delete()
      assert.is_nil(getWindowGeometry("gasContaineradjLabel"))
      assert.is_nil(Geyser.windowList.gasContainer)
    end)
  end)
end)
