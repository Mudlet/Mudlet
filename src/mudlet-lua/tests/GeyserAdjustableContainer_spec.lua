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

  -- Adjustable.Container.Attached is keyed by container name, so two live
  -- containers sharing a name land on the same key. resetBorder/adjustBorder
  -- walk those entries to work out how much border to reserve, so a container
  -- that is still attached but no longer registered loses its reservation and
  -- the main console is drawn underneath it.
  describe("Tests the functionality of Adjustable.Container:attachToBorder/detach", function()
    local containers
    local borderBefore

    local function make(name, width)
      local container = Adjustable.Container:new({
        name = name,
        x = 0, y = 0, width = width, height = 100,
        autoLoad = false,
        autoSave = false,
      })
      containers[#containers + 1] = container
      return container
    end

    before_each(function()
      containers = {}
      borderBefore = getBorderLeft()
    end)

    -- Deliberately same named containers share their children's names too, so
    -- the one that still holds the registration is deleted first and takes the
    -- widgets with it; the superseded one is then deleted for its own event
    -- handlers and bookkeeping, which nothing else would clear.
    after_each(function()
      for index = #containers, 1, -1 do
        local container = containers[index]
        if container.attached then
          container:detach()
        end
        container:delete()
      end
      containers = {}
      setBorderLeft(borderBefore)
      assert.is_nil(Adjustable.Container.Attached.left.gasAttachPlain)
      assert.is_nil(Adjustable.Container.Attached.left.gasAttachName)
      assert.is_nil(Adjustable.Container.Attached.left.gasDetachName)
    end)

    it("reserves a border while attached and gives it back on detach", function()
      local container = make("gasAttachPlain", 200)
      container:attachToBorder("left")
      assert.are.equal("left", container.attached)
      assert.are.equal(container.borderSize, getBorderLeft())
      assert.are.equal(container, Adjustable.Container.Attached.left.gasAttachPlain)
      container:detach()
      assert.is_false(container.attached)
      assert.is_nil(Adjustable.Container.Attached.left.gasAttachPlain)
      assert.are.equal(0, getBorderLeft())
    end)

    it("detaches a same named container it takes the registration from", function()
      local first = make("gasAttachName", 200)
      local second = make("gasAttachName", 400)
      first:attachToBorder("left")
      assert.are.equal(first.borderSize, getBorderLeft())
      second:attachToBorder("left")
      assert.are.equal(second, Adjustable.Container.Attached.left.gasAttachName)
      assert.are.equal(second.borderSize, getBorderLeft())
      -- the superseded container must not be left believing it is attached
      -- while nothing reserves a border for it any more
      assert.is_false(first.attached)
      assert.is_nil(first.borderSize)
    end)

    it("leaves a same named container's reservation alone when a superseded one detaches", function()
      local first = make("gasDetachName", 200)
      local second = make("gasDetachName", 400)
      first:attachToBorder("left")
      second:attachToBorder("left")
      local reserved = getBorderLeft()
      first:detach()
      assert.are.equal(second, Adjustable.Container.Attached.left.gasDetachName)
      assert.are.equal("left", second.attached)
      assert.are.equal(reserved, getBorderLeft())
    end)
  end)
end)

-- The handlers Adjustable.Container hangs off its own labels. A real mouse is
-- what normally calls them, with the event table Mudlet builds for a label
-- callback ({button = ..., x = ..., y = ..., globalX = ..., globalY = ...}), so
-- these specs hand them that table directly and read back what they did.
describe("Tests the Adjustable.Container mouse handlers", function()
  local container
  local containerName = "gahContainer"

  local function mouseEvent(button, x, y)
    x, y = x or 5, y or 5
    return {button = button, buttons = {button}, x = x, y = y, globalX = x, globalY = y}
  end

  before_each(function()
    container = Adjustable.Container:new({
      name = containerName,
      x = 20, y = 30, width = 200, height = 200,
      autoLoad = false,
      autoSave = false,
    })
    -- Adjustable.Container keeps which edge is being dragged in one table
    -- shared by every container, and only a completed left click empties it,
    -- so start each spec from a released mouse rather than from whatever the
    -- last spec left mid-drag
    container:onClick(container.adjLabel, mouseEvent("LeftButton", 100, 100))
    container:onRelease(container.adjLabel, mouseEvent("LeftButton", 100, 100))
  end)

  after_each(function()
    -- onEnterAtt and the right click path both open a nest, which arms a timer
    -- that would fire on deleted labels seconds later
    if Geyser.Label.closeAllTimer then
      killTimer(Geyser.Label.closeAllTimer)
      Geyser.Label.closeAllTimer = nil
    end
    if container then
      -- and leave the drag state pointing at nothing rather than at a label
      -- about to be deleted: Adjustable.Container:reposition reads it. A locked
      -- container refuses the click, so unlock before making it
      if container.locked then
        container:unlockContainer()
      end
      container:onClick(container.adjLabel, mouseEvent("LeftButton", 100, 100))
      container:onRelease(container.adjLabel, mouseEvent("LeftButton", 100, 100))
      for _, label in ipairs({container.adjLabel, container.attLabel, container.rCLabel}) do
        if label then
          -- keyed by the label object, so an entry outlives the label
          Geyser.Label.scrollV[label] = nil
          Geyser.Label.scrollH[label] = nil
        end
      end
      container:deleteSaveFile()
      if Geyser.windowList[containerName] == container then
        container:delete()
      end
    end
    container = nil
    Adjustable.Container.all[containerName] = nil
    local index = table.index_of(Adjustable.Container.all_windows, containerName)
    if index then
      table.remove(Adjustable.Container.all_windows, index)
    end
  end)

  describe("Adjustable.Container:onClick and onRelease", function()
    it("raises the reposition event once a left click is let go of", function()
      local seen
      local handler = registerAnonymousEventHandler("AdjustableContainerRepositionFinish",
        function(_, name, width, height, x, y) seen = {name, width, height, x, y} end)
      finally(function() killAnonymousEventHandler(handler) end)

      container:onClick(container.adjLabel, mouseEvent("LeftButton"))
      container:onRelease(container.adjLabel, mouseEvent("LeftButton"))

      assert.is_table(seen)
      assert.are.same({containerName, container:get_width(), container:get_height(), container:get_x(), container:get_y()}, seen)
    end)

    it("stays quiet when the release was not of a left click", function()
      local raised = false
      local handler = registerAnonymousEventHandler("AdjustableContainerRepositionFinish", function() raised = true end)
      finally(function() killAnonymousEventHandler(handler) end)

      container:onClick(container.adjLabel, mouseEvent("LeftButton"))
      container:onRelease(container.adjLabel, mouseEvent("RightButton"))

      assert.is_false(raised)
    end)

    it("stays quiet for a label that was not the one clicked", function()
      local raised = false
      local handler = registerAnonymousEventHandler("AdjustableContainerRepositionFinish", function() raised = true end)
      finally(function() killAnonymousEventHandler(handler) end)

      container:onClick(container.adjLabel, mouseEvent("LeftButton"))
      container:onRelease(container.exitLabel, mouseEvent("LeftButton"))

      assert.is_false(raised)
    end)

    it("only raises the event once per click", function()
      local raises = 0
      local handler = registerAnonymousEventHandler("AdjustableContainerRepositionFinish", function() raises = raises + 1 end)
      finally(function() killAnonymousEventHandler(handler) end)

      container:onClick(container.adjLabel, mouseEvent("LeftButton"))
      container:onRelease(container.adjLabel, mouseEvent("LeftButton"))
      container:onRelease(container.adjLabel, mouseEvent("LeftButton"))

      assert.are.equal(1, raises)
    end)

    it("takes the grabbing hand back after the drag", function()
      container.adjLabel:setCursor("ClosedHand")
      container:onClick(container.adjLabel, mouseEvent("LeftButton"))
      container:onRelease(container.adjLabel, mouseEvent("LeftButton"))
      assert.are.equal("OpenHand", container.adjLabel.cursorShape)
    end)

    it("ignores a left click on a locked container that is on its own", function()
      local raised = false
      local handler = registerAnonymousEventHandler("AdjustableContainerRepositionFinish", function() raised = true end)
      finally(function() killAnonymousEventHandler(handler) end)

      container:lockContainer()
      container:onClick(container.adjLabel, mouseEvent("LeftButton"))
      container:onRelease(container.adjLabel, mouseEvent("LeftButton"))

      -- the click never registered, so there is no drag to finish
      assert.is_false(raised)
    end)
  end)

  describe("Adjustable.Container:onMove", function()
    it("turns the container's position into a percentage of the main window", function()
      local originalX, originalY = container:get_x(), container:get_y()

      container:onClick(container.adjLabel, mouseEvent("LeftButton"))
      container:onMove(container.adjLabel, mouseEvent("LeftButton"))

      -- the mouse has not actually moved between the two, so the container
      -- lands where it already was, but now expressed against the window
      assert.is_truthy(tostring(container.x):find("%%$"))
      assert.is_truthy(tostring(container.y):find("%%$"))
      assert.is_true(math.abs(container:get_x() - originalX) <= 1)
      assert.is_true(math.abs(container:get_y() - originalY) <= 1)
      -- moving does not touch the size, which is what tells this apart from
      -- the resize branch below
      assert.are.equal("200px", container.width)
      assert.are.equal("200px", container.height)
    end)

    it("shows the grabbing hand while the container is being dragged", function()
      container:onClick(container.adjLabel, mouseEvent("LeftButton"))
      container:onMove(container.adjLabel, mouseEvent("LeftButton"))
      assert.are.equal("ClosedHand", container.adjLabel.cursorShape)
    end)

    it("takes the cursor away again and moves nothing while locked", function()
      container.adjLabel:setCursor("OpenHand")
      container:lockContainer()

      container:onMove(container.adjLabel, mouseEvent("NoButton"))

      assert.are.equal(0, container.adjLabel.cursorShape)
      -- still the pixel position the constructor was given: a drag would have
      -- made it a percentage of the main window, whatever pixel that works out
      -- to. Geyser rewrites the plain number into "20px" when it constrains it
      assert.are.equal("20px", container.x)
    end)

    -- a click that lands within ten pixels of an edge grabs that edge, and the
    -- next click sees it and switches from moving to resizing
    it("resizes rather than moves when the drag started on an edge", function()
      container:onClick(container.adjLabel, mouseEvent("LeftButton", 5, 5))
      container:onClick(container.adjLabel, mouseEvent("LeftButton", 5, 5))
      container:onMove(container.adjLabel, mouseEvent("LeftButton", 5, 5))

      -- resizing rewrites the size as well as the position, where moving left
      -- the size alone
      assert.is_truthy(tostring(container.width):find("%%$"))
      assert.is_truthy(tostring(container.height):find("%%$"))
      -- the mouse did not move, so the left edge it grabbed stays where it was
      assert.is_true(math.abs(container:get_width() - 200) <= 1)
    end)
  end)

  describe("Adjustable.Container:onClickL", function()
    it("locks an unlocked container and hides its buttons", function()
      assert.is_false(container.locked)
      container:onClickL()
      assert.is_true(container.locked)
      assert.is_false(windowVisible(container.exitLabel.name))
      assert.is_false(windowVisible(container.minimizeLabel.name))
    end)

    it("unlocks a locked one and gives the buttons back", function()
      container:onClickL()
      container:onClickL()
      assert.is_false(container.locked)
      assert.is_true(windowVisible(container.exitLabel.name))
      assert.is_true(windowVisible(container.minimizeLabel.name))
    end)
  end)

  describe("Adjustable.Container:onClickMin", function()
    it("minimizes an open container down to its title bar", function()
      assert.is_false(container.minimized)
      container:onClickMin()
      assert.is_true(container.minimized)
      -- Inside is a plain Geyser.Container with no widget of its own, so its
      -- own flag is the only place its visibility is readable
      assert.is_true(container.Inside.hidden)
      assert.are.equal(container.buttonsize + 10, container:get_height())
    end)

    it("restores a minimized one to the height it had", function()
      local originalHeight = container:get_height()
      container:onClickMin()
      container:onClickMin()
      assert.is_false(container.minimized)
      assert.is_false(container.Inside.hidden)
      assert.are.equal(originalHeight, container:get_height())
    end)
  end)

  describe("Adjustable.Container:onClickSave and onClickLoad", function()
    local saveFile

    before_each(function()
      saveFile = string.format("%s%s.lua", container.defaultDir, containerName)
      container:deleteSaveFile()
    end)

    it("writes the container's layout to its save file", function()
      assert.is_false(io.exists(saveFile))
      container:onClickSave()
      assert.is_true(io.exists(saveFile))
    end)

    it("puts a saved layout back over whatever the container has now", function()
      container:onClickSave()
      container:move(300, 400)
      assert.are.equal(300, container:get_x())

      container:onClickLoad()

      assert.are.equal(20, container:get_x())
      assert.are.equal(30, container:get_y())
    end)

    it("brings the locked state back with the layout", function()
      container:onClickL()
      container:onClickSave()
      container:onClickL()
      assert.is_false(container.locked)

      container:onClickLoad()

      assert.is_true(container.locked)
    end)

    it("does nothing to a container that has never been saved", function()
      container:move(300, 400)
      container:onClickLoad()
      assert.are.equal(300, container:get_x())
    end)
  end)

  describe("Adjustable.Container:onEnterAtt", function()
    it("fills the attach menu with the borders the container can reach", function()
      local positions = container:validAttachPositions()
      assert.is_true(#positions > 0, "a container at the top left should be able to attach somewhere")

      container:onEnterAtt()

      assert.are.equal(#positions, #container.attLabel.nestedLabels)
      for index = 1, #positions do
        assert.are.equal(container.att[index], container.attLabel.nestedLabels[index])
        assert.are.equal("Adjustable.Container.attachToBorder", container.att[index].clickCallback)
      end
    end)

    it("opens the menu it just built", function()
      container:onEnterAtt()
      assert.is_true(windowVisible(container.att[1].name))
    end)

    it("rebuilds the menu rather than adding to it when hovered again", function()
      container:onEnterAtt()
      local first = #container.attLabel.nestedLabels
      container:onEnterAtt()
      assert.are.equal(first, #container.attLabel.nestedLabels)
    end)

    it("drops the borders the container has moved away from", function()
      -- the container starts at (20, 30), within reach of the top and left
      assert.is_truthy(table.contains(container:validAttachPositions(), "top"))
      assert.is_truthy(table.contains(container:validAttachPositions(), "left"))

      local winWidth, winHeight = getMainWindowSize()
      container:move(winWidth * 0.5, winHeight * 0.5)
      local reachable = container:validAttachPositions()
      -- half a window away is out of reach of both, whatever the window size
      assert.is_false(table.contains(reachable, "top"))
      assert.is_false(table.contains(reachable, "left"))

      container:onEnterAtt()

      assert.are.equal(#reachable, #container.attLabel.nestedLabels)
    end)
  end)
end)

-- Dragging a container out of the container it is nested in. onMove follows the
-- real mouse pointer through getMousePosition(), which a headless run cannot
-- drive, so these specs go at the two functions onMove hands a drag to: the
-- decision that the pointer has left the parent, and the move out itself.
describe("Tests dragging an Adjustable.Container out of its parent", function()
  local dock, child
  local dockName, childName = "gadDock", "gadChild"

  local function makeChild(cons)
    cons = cons or {}
    cons.name = childName
    cons.x, cons.y = cons.x or 20, cons.y or 20
    cons.width, cons.height = cons.width or "50%", cons.height or "50%"
    cons.autoLoad, cons.autoSave = false, false
    child = Adjustable.Container:new(cons, dock)
    return child
  end

  local function assertNear(actual, expected, what)
    assert.is_true(math.abs(actual - expected) <= 1,
      string.format("%s: expected %s, got %s", what, tostring(expected), tostring(actual)))
  end

  -- drives a drag through the handlers a mouse would call. The handlers ask
  -- getMousePosition where the pointer is, and they look it up in the globals of
  -- the file they live in, which is not the globals a spec file writes to, so the
  -- stand-in for the mouse has to go into their own environment
  local function drag(container, grabX, grabY, stepX, stepY, steps)
    local geyser = getfenv(Adjustable.Container.onMove)
    local realGetMousePosition = geyser.getMousePosition
    local pointerX, pointerY = 500, 400
    local event = {button = "LeftButton", buttons = {"LeftButton"}, x = grabX, y = grabY, globalX = pointerX, globalY = pointerY}
    geyser.getMousePosition = function() return pointerX, pointerY end
    local ok, err = pcall(function()
      -- which edge is being dragged is kept in one table shared by every
      -- container, and only a completed click empties it, so start from a
      -- released mouse rather than from whatever the last spec left behind
      container:onClick(container.adjLabel, event)
      container:onRelease(container.adjLabel, event)
      container:onClick(container.adjLabel, event)
      for _ = 1, steps do
        pointerX, pointerY = pointerX + stepX, pointerY + stepY
        container:onMove(container.adjLabel, event)
      end
      container:onRelease(container.adjLabel, event)
    end)
    geyser.getMousePosition = realGetMousePosition
    if not ok then
      error(err, 0)
    end
  end

  before_each(function()
    -- the window is whatever size the machine running the specs gives them, so
    -- the container everything else nests in is sized to fit in it rather than
    -- assumed to. Everything below goes by what it reads back, not by these
    local winWidth, winHeight = getMainWindowSize()
    local width, height = math.min(400, winWidth - 100), math.min(300, winHeight - 100)
    assert.is_true(width >= 200 and height >= 150,
      string.format("these specs need a window with room for a container in it, this one is %dx%d", winWidth, winHeight))
    dock = Adjustable.Container:new({
      name = dockName,
      x = 50, y = 50, width = width, height = height,
      autoLoad = false,
      autoSave = false,
    })
  end)

  local function cleanUp(container)
    if container and Adjustable.Container.all[container.name] == container then
      container:deleteSaveFile()
      container:delete()
    end
  end

  after_each(function()
    -- the child first: dragged out it is no longer the dock's to delete, and
    -- still inside it the dock's cascade would have taken it along
    cleanUp(child)
    cleanUp(dock)
    child, dock = nil, nil
  end)

  it("holds a nested container inside its parent by default", function()
    makeChild()
    -- a container puts its children in an inside container of its own, which is
    -- the parent a nested container is held inside of
    assert.are.equal(dock.Inside, child.container)
    assert.is_false(child.dragOut)
    assert.is_false(child:dragLeavesParent(-500, -500, 5, 5))
  end)

  it("stays inside while the pointer is still over the parent", function()
    makeChild({dragOut = true})
    local inside = child.container
    assert.is_false(child:dragLeavesParent(0, 0, 5, 5))
    assert.is_false(child:dragLeavesParent(inside.get_width() - 50, inside.get_height() - 50, 5, 5))
  end)

  it("puts up with a small overshoot at the parent's edge", function()
    makeChild({dragOut = true})
    -- pushing a container flush into the corner of its parent and going a little
    -- too far, which is not what asking for it to come out looks like
    assert.is_false(child:dragLeavesParent(-25, 0, 5, 5))
  end)

  it("stays in while the pointer is over the parent's own title bar", function()
    makeChild({dragOut = true})
    -- a container holds its children below its title bar, so a pointer over that
    -- title bar is already outside the area the children are placed in
    dock:setPadding(30)
    assert.is_false(child:dragLeavesParent(-40, -40, 5, 5))
    assert.is_true(child:dragLeavesParent(-40, -120, 5, 5))
  end)

  it("goes by where the pointer is rather than by where the container was pushed to", function()
    makeChild({dragOut = true, width = "80%"})
    local inside = child.container
    local grabX = child:get_width() - 20

    -- the drag asking for far to the left of the parent, but the container was
    -- grabbed near its right hand end, so the pointer is still over the parent
    assert.is_false(child:dragLeavesParent(-100, 10, grabX, 5))
    -- and the other way about: the container barely past the right hand edge,
    -- but the pointer that dragged it there is well clear of the parent
    assert.is_true(child:dragLeavesParent(inside.get_width() - 50, 10, grabX, 5))
  end)

  it("comes out once the pointer is dragged clear of the parent", function()
    makeChild({dragOut = true})
    local inside = child.container
    assert.is_true(child:dragLeavesParent(-100, 0, 5, 5))
    assert.is_true(child:dragLeavesParent(0, -100, 5, 5))
    assert.is_true(child:dragLeavesParent(inside.get_width() + 100, 0, 5, 5))
    assert.is_true(child:dragLeavesParent(0, inside.get_height() + 100, 5, 5))
  end)

  it("leaves a container that is not nested in another one alone", function()
    makeChild({dragOut = true})
    child:changeContainer(Geyser)
    assert.is_false(child:dragLeavesParent(-500, -500, 5, 5))
  end)

  it("holds on to an attached container", function()
    makeChild({dragOut = true})
    -- set rather than attached for real: attachToBorder only takes a container
    -- near enough to a border, which depends on the size of the window the tests
    -- happen to run in, and it reserves a main window border and registers a
    -- resize handler that a failing spec would leave behind
    finally(function() child.attached = false end)
    child.attached = "left"
    assert.is_false(child:dragLeavesParent(-500, -500, 5, 5))
    child.attached = false
    assert.is_true(child:dragLeavesParent(-500, -500, 5, 5))
  end)

  it("stays where it is on screen when it comes out of its parent", function()
    makeChild({dragOut = true})
    local inside = child.container
    local x, y = child:get_x(), child:get_y()
    local width, height = child:get_width(), child:get_height()

    -- the position the drag asks for is the one the container is already at
    assert.is_true(child:dragOutOfParent(x - inside.get_x(), y - inside.get_y()))

    assert.are.equal(Geyser, child.container)
    assert.are.equal(child, Geyser.windowList[childName])
    assert.is_true(child.draggedOut)
    assertNear(child:get_x(), x, "x")
    assertNear(child:get_y(), y, "y")
    assertNear(child:get_width(), width, "width")
    assertNear(child:get_height(), height, "height")
    -- and the widget went with it rather than only the bookkeeping
    local widgetX, widgetY = getWindowGeometry(childName .. "adjLabel")
    assertNear(widgetX, x, "widget x")
    assertNear(widgetY, y, "widget y")
  end)

  it("follows the drag out to where the pointer took it", function()
    makeChild({dragOut = true})
    local inside = child.container
    local insideX, insideY = inside.get_x(), inside.get_y()

    child:dragOutOfParent(-50, 10)

    assertNear(child:get_x(), insideX - 50, "x")
    assertNear(child:get_y(), insideY + 10, "y")
  end)

  it("keeps the size it had, whether that was a share of the parent or pixels", function()
    makeChild({dragOut = true, width = "50%", height = 100})
    local width = child:get_width()

    child:dragOutOfParent(0, 0)

    assert.is_truthy(tostring(child.width):find("%%$"))
    assertNear(child:get_width(), width, "width")
    assert.are.equal("100px", child.height)
    assert.are.equal(100, child:get_height())
  end)

  it("keeps the size of a container measured from its parent's far edge", function()
    -- a negative size is the parent's, less that many pixels, so it means a
    -- different size in the window the container comes out into
    makeChild({dragOut = true, width = "-100"})
    local width = child:get_width()

    child:dragOutOfParent(0, 0)

    assert.is_truthy(tostring(child.width):find("%%$"))
    assertNear(child:get_width(), width, "width")
  end)

  it("keeps a container that comes out at the edge inside the window", function()
    makeChild({dragOut = true})
    local winWidth, winHeight = getMainWindowSize()

    child:dragOutOfParent(-100000, -100000)
    assert.are.equal(0, child:get_x())
    assert.are.equal(0, child:get_y())

    child:changeContainer(dock.Inside)
    child:dragOutOfParent(100000, 100000)
    assertNear(child:get_x(), winWidth - child:get_width(), "x")
    assertNear(child:get_y(), winHeight - child:get_height(), "y")
  end)

  it("restores a minimized container to the height it had", function()
    makeChild({dragOut = true})
    local height = child:get_height()
    child:minimize()

    child:dragOutOfParent(0, 0)
    child:restore()

    assert.is_false(child.minimized)
    -- the height it restores to was a share of the parent it has left
    assertNear(child:get_height(), height, "restored height")
  end)

  it("does nothing for a container that is not nested in another one", function()
    makeChild({dragOut = true})
    child:changeContainer(Geyser)
    local x = child:get_x()

    assert.is_false(child:dragOutOfParent(-500, -500))

    assert.are.equal(x, child:get_x())
  end)

  it("is not dragged out any more once a script puts it back in a container", function()
    makeChild({dragOut = true})
    child:dragOutOfParent(0, 0)
    assert.is_true(child.draggedOut)

    child:changeContainer(dock.Inside)

    assert.are.equal(dock.Inside, child.container)
    assert.is_false(child.draggedOut)
  end)

  it("comes back out of its parent when its saved settings are loaded", function()
    makeChild({dragOut = true})
    child:dragOutOfParent(-50, 10)
    local x, y = child:get_x(), child:get_y()
    child:save()
    child:delete()

    local reloaded = makeChild({dragOut = true})
    assert.are.equal(dock.Inside, reloaded.container)
    reloaded:load()

    assert.are.equal(Geyser, reloaded.container)
    assert.is_true(reloaded.draggedOut)
    assertNear(reloaded:get_x(), x, "x")
    assertNear(reloaded:get_y(), y, "y")
  end)

  it("restores a minimized container that was dragged out to the height it had", function()
    makeChild({dragOut = true})
    local height = child:get_height()
    child:minimize()
    child:dragOutOfParent(-50, 10)
    child:save()
    child:delete()

    local reloaded = makeChild({dragOut = true})
    reloaded:load()
    reloaded:restore()

    assert.are.equal(Geyser, reloaded.container)
    assertNear(reloaded:get_height(), height, "restored height")
  end)

  it("goes back into its parent for settings that were saved before it was dragged out", function()
    makeChild({dragOut = true})
    local x, y = child:get_x(), child:get_y()
    child:save()

    child:dragOutOfParent(-50, 10)
    child:load()

    -- those settings are a position inside the parent, which is only that
    -- position while the container is back inside the parent
    assert.are.equal(dock.Inside, child.container)
    assert.is_false(child.draggedOut)
    assertNear(child:get_x(), x, "x")
    assertNear(child:get_y(), y, "y")
  end)

  it("comes out into the user window it is in rather than into the main window", function()
    local userWindow = Geyser.UserWindow:new({name = "gadUserWindow", x = 0, y = 0, width = 200, height = 200})
    finally(function()
      -- the user window's own root container is what takes the window and
      -- everything left in it with it
      local root = Geyser.windowList.gadUserWindowContainer
      if root then
        root:delete()
      end
    end)
    local windowDock = Adjustable.Container:new({
      name = "gadWindowDock",
      x = 0, y = 0, width = 150, height = 150,
      autoLoad = false, autoSave = false,
    }, userWindow)
    local windowChild = Adjustable.Container:new({
      name = "gadWindowChild",
      x = 10, y = 10, width = 100, height = 60,
      autoLoad = false, autoSave = false,
      dragOut = true,
    }, windowDock)
    assert.are.equal(windowDock.Inside, windowChild.container)

    assert.is_true(windowChild:dragOutOfParent(0, 0))

    assert.are.equal(userWindow, windowChild.container)
    assert.are.equal("gadUserWindow", windowChild.windowname)
  end)

  it("leaves a container without dragOut where the package that made it put it", function()
    makeChild({dragOut = true})
    child:dragOutOfParent(-50, 10)
    child:save()
    child:delete()

    local reloaded = makeChild()
    reloaded:load()

    assert.are.equal(dock.Inside, reloaded.container)
  end)

  it("comes out to the window rather than to the container in between", function()
    local middle, deep
    finally(function()
      cleanUp(deep)
      cleanUp(middle)
    end)
    middle = Adjustable.Container:new({
      name = "gadMiddle",
      x = 10, y = 10, width = "80%", height = "80%",
      autoLoad = false, autoSave = false,
    }, dock)
    deep = Adjustable.Container:new({
      name = "gadDeep",
      x = 5, y = 5, width = 60, height = 40,
      autoLoad = false, autoSave = false,
      dragOut = true,
    }, middle)
    assert.are.equal(middle.Inside, deep.container)
    local x, y = deep:get_x(), deep:get_y()

    assert.is_true(deep:dragOutOfParent(x - middle.Inside.get_x(), y - middle.Inside.get_y()))

    assert.are.equal(Geyser, deep.container)
    -- the position of a container two levels down counts in every container it
    -- is inside of, so it has to come out of all of them onto the same spot
    assertNear(deep:get_x(), x, "x")
    assertNear(deep:get_y(), y, "y")
  end)

  it("keeps a container bigger than the window at the window's corner", function()
    local winWidth, winHeight = getMainWindowSize()
    makeChild({dragOut = true, width = winWidth + 200, height = winHeight + 200})

    child:dragOutOfParent(0, 0)

    -- there is nowhere to put a container that does not fit that keeps all of it
    -- on screen, so it goes at the corner rather than off the top left of it
    assert.are.equal(0, child:get_x())
    assert.are.equal(0, child:get_y())
  end)

  it("does not hold a container back at the edge of a scroll box", function()
    local box
    finally(function()
      -- deleting the box takes everything that was put in it with it
      if box then
        box:delete()
      end
    end)
    box = Geyser.ScrollBox:new({name = "gadScrollBox", x = 0, y = 0, width = 200, height = 200})
    local boxDock = Adjustable.Container:new({
      name = "gadBoxDock",
      x = 0, y = 0, width = 150, height = 150,
      autoLoad = false, autoSave = false,
    }, box)
    local boxChild = Adjustable.Container:new({
      name = "gadBoxChild",
      x = 10, y = 10, width = 60, height = 40,
      autoLoad = false, autoSave = false,
      dragOut = true,
    }, boxDock)
    assert.are.equal("gadScrollBox", boxChild.windowname)

    assert.is_true(boxChild:dragOutOfParent(400, 400))

    assert.are.equal(box, boxChild.container)
    -- a scroll box is scrolled to what does not fit into it, so a container
    -- dragged out beyond its edge is not pulled back to it
    assert.is_true(boxChild:get_x() > box.get_width() - boxChild:get_width())
  end)

  it("hands back what Geyser makes of a container it will not move into", function()
    makeChild({dragOut = true})

    local moved, message = child:changeContainer(nil)

    assert.is_nil(moved)
    assert.is_string(message)
    assert.are.equal(dock.Inside, child.container)
  end)

  it("carries a drag that keeps pushing past the parent out of it", function()
    makeChild({dragOut = true})
    local inside = child.container

    -- grabbed 60 pixels in and dragged 200 to the left, which is far enough for
    -- the pointer to leave the parent long after the container stopped at its edge
    drag(child, 60, 30, -10, 0, 20)

    assert.are.equal(Geyser, child.container)
    assert.is_true(child.draggedOut)
    assert.is_true(child:get_x() < inside.get_x(), "the container ended up outside its old parent")
  end)

  it("holds a drag inside the parent when the container was not given dragOut", function()
    makeChild()
    local inside = child.container

    drag(child, 60, 30, -10, 0, 20)

    assert.are.equal(inside, child.container)
    -- held against the inside of its parent, which is what a drag out is opting
    -- out of and every container without the constraint still does
    assertNear(child:get_x(), inside.get_x(), "x")
    assert.is_truthy(tostring(child.x):find("%%$"))
  end)

  it("does not take a locked container out of its parent", function()
    makeChild({dragOut = true})
    local inside = child.container
    local x = child:get_x()
    child:lockContainer()

    drag(child, 60, 30, -10, 0, 20)

    assert.are.equal(inside, child.container)
    assert.are.equal(x, child:get_x())
  end)
end)
