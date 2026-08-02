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
      -- deleting rather than hiding keeps the container, its right click menu
      -- and the submenu addConnectMenu builds out of every later spec file
      if testContainer and Geyser.windowList.testAdjustableContainer == testContainer then
        testContainer:delete()
      end
      testContainer = nil
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

      ac:delete()
    end)
  end)

  -- Geometry, visibility and title readback, asserted on the widgets the
  -- container builds rather than on its bookkeeping alone.
  describe("Adjustable.Container widget state", function()
    local container
    local topLevelBefore

    local function geometry(name)
      local x, y, width, height = getWindowGeometry(name)
      return {x = x, y = y, width = width, height = height}
    end

    -- Every top level Geyser object registered since this spec's container was
    -- built. Snapshotting rather than matching on the container's name catches
    -- leaks whatever they are called, such as the menu's "More..." labels.
    local function newTopLevelObjects()
      local new = {}
      for name in pairs(Geyser.windowList) do
        if not topLevelBefore[name] then
          new[#new + 1] = name
        end
      end
      table.sort(new)
      return new
    end

    before_each(function()
      topLevelBefore = {}
      for name in pairs(Geyser.windowList) do
        topLevelBefore[name] = true
      end
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
      -- a delete that throws must not skip the sweep below, or it strands the
      -- container and its menu labels for the rest of the suite
      local deleted, deleteError = true, nil
      if container and Geyser.windowList.gasContainer == container then
        deleted, deleteError = pcall(function() container:delete() end)
      end
      container = nil
      -- the whole suite shares one Lua state, so anything left registered here
      -- would follow later spec files around: sweep it, but report it rather
      -- than quietly repairing a delete that stopped cleaning up after itself
      local leftovers = newTopLevelObjects()
      for _, name in ipairs(leftovers) do
        local object = Geyser.windowList[name]
        if object then
          object:delete()
        end
      end
      Adjustable.Container.all.gasContainer = nil
      local index = table.index_of(Adjustable.Container.all_windows, "gasContainer")
      if index then
        table.remove(Adjustable.Container.all_windows, index)
      end
      -- the delete throwing is the root cause, so report it ahead of the leak
      -- it would have caused
      if not deleted then
        error(deleteError)
      end
      assert.are.same({}, leftovers)
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

    it("puts a child inside the padding and below the title bar", function()
      Geyser.Label:new({name = "gasChild", x = 0, y = 0, width = "100%", height = "100%"}, container)
      -- padding on the left and right, twice that at the top to leave room for
      -- the title bar
      assert.are.equal(10, container.padding)
      assert.are.same({x = 30, y = 50, width = 180, height = 170}, geometry("gasChild"))
    end)

    it("setPadding moves and resizes the children", function()
      Geyser.Label:new({name = "gasPaddedChild", x = 0, y = 0, width = "100%", height = "100%"}, container)
      container:setPadding(30)
      assert.are.equal(30, container.padding)
      assert.are.same({x = 50, y = 90, width = 140, height = 110}, geometry("gasPaddedChild"))
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
      -- buttonsize is stored as a string, hence the conversion
      assert.are.equal(tonumber(container.buttonsize) + 10, minimized.height)
      container:restore()
      assert.is_false(container.minimized)
      assert.are.same({x = 20, y = 30, width = 200, height = 200}, geometry("gasContaineradjLabel"))
    end)

    it("titles itself after its name again after resetTitle", function()
      container:setTitle("My Title", "red", "c")
      container:resetTitle()
      assert.are.equal("gasContainer - Adjustable Container", container.titleText)
      -- back to what the constructor produced, colour and alignment included
      assert.are.equal("grey", container.titleTxtColor)
      assert.are.equal("l", container.titleFormat)
      local text = getLabelText("gasContaineradjLabel")
      assert.is_truthy(text:find("gasContainer - Adjustable Container", 1, true))
      assert.is_truthy(text:find("color: " .. Geyser.Color.hex("grey"), 1, true))
    end)

    it("deletes the container and its backdrop label", function()
      container:delete()
      assert.is_nil(getWindowGeometry("gasContaineradjLabel"))
      assert.is_nil(getWindowGeometry("gasContainerexitLabel"))
      assert.is_nil(Geyser.windowList.gasContainer)
    end)

    it("takes its right click menu labels and its registration with it", function()
      -- menu labels are registered as top level Geyser objects rather than as
      -- children of the menu, so only the container's own delete reaches them
      local menuLabelName = container.lockLabel.name
      local lockStyleLabelName = container.adjLabel:findMenuElement("lockStylesLabel.standard").name
      assert.is_not_nil(Geyser.windowList[menuLabelName])
      assert.is_not_nil(Geyser.windowList[lockStyleLabelName])
      container:delete()
      -- listed by name: a leaked Geyser object prints as the whole widget tree
      assert.are.same({}, newTopLevelObjects())
      assert.is_nil(getWindowGeometry(menuLabelName))
      assert.is_nil(getWindowGeometry(lockStyleLabelName))
      assert.is_nil(Adjustable.Container.all.gasContainer)
      assert.is_nil(table.index_of(Adjustable.Container.all_windows, "gasContainer"))
    end)

    it("takes the menu labels of a container inside a user window with it", function()
      -- menu labels of a container in a user window are registered in that
      -- window's list rather than in Geyser.windowList
      local userWindow = Geyser.UserWindow:new({name = "gasUserWindow", x = 0, y = 0, width = 300, height = 300})
      finally(function()
        -- a user window gets a root container of its own, which is what has to
        -- go for the window and everything in it to be cleaned up
        local root = Geyser.windowList.gasUserWindowContainer
        if root then
          root:delete()
        end
      end)
      local inWindow = Adjustable.Container:new({
        name = "gasInUserWindow",
        x = 0, y = 0, width = 100, height = 100,
        autoLoad = false,
        autoSave = false,
      }, userWindow)
      local menuLabelName = inWindow.lockLabel.name
      assert.is_not_nil(getWindowGeometry(menuLabelName))
      inWindow:delete()
      assert.is_nil(getWindowGeometry(menuLabelName))
    end)

    it("takes its autosave handler with it", function()
      local saving = Adjustable.Container:new({
        name = "gasSavingContainer",
        x = 0, y = 0, width = 100, height = 100,
        autoLoad = false,
      })
      assert.is_not_nil(saving.autoSaveHandler)
      saving:delete()
      -- left registered, the handler would write a deleted container's geometry
      -- back out at exit, over whatever took its name in the meantime
      assert.is_nil(saving.autoSaveHandler)
      assert.is_false(saving.autoSave)
    end)

    it("leaves another adjustable container's registration alone", function()
      local other = Adjustable.Container:new({
        name = "gasOtherContainer",
        x = 0, y = 0, width = 100, height = 100,
        autoLoad = false,
        autoSave = false,
      })
      finally(function()
        if Geyser.windowList.gasOtherContainer == other then
          other:delete()
        end
      end)
      local otherMenuLabelName = other.lockLabel.name
      container:delete()
      assert.are.equal(other, Adjustable.Container.all.gasOtherContainer)
      assert.is_not_nil(table.index_of(Adjustable.Container.all_windows, "gasOtherContainer"))
      assert.are.equal(other.lockLabel, Geyser.windowList[otherMenuLabelName])
      assert.is_not_nil(getWindowGeometry(otherMenuLabelName))
      other:delete()
    end)
  end)
end)
