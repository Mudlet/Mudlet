--------------------------------------
--                                  --
-- The Geyser Layout Manager by guy --
--                                  --
--------------------------------------

--==================================================
-- Create the Geyser Root Container
--==================================================

--- Create the Geyser namespace.
-- Geyser is considered to be a "container window" for the purposes
-- of default limits for new windows, in fact, it is the root container
-- window, and is the metatable for Geyser.Container. It has the
-- minimum number of functions needed to behave as a Container.
Geyser = Geyser or { i = 0, x = 0, y = 0 }

--Layout policies. Fixed = Do not stretch/shrink, Dynamic = Do stretch/shrink
Geyser.Fixed, Geyser.Dynamic = 0, 1

Geyser.width, Geyser.height = getMainWindowSize()

Geyser.get_x = function()
  return 0
end
Geyser.get_y = function()
  return 0
end
Geyser.get_width = function()
  return getMainWindowSize()
end
Geyser.get_height = function()
  local w, h = getMainWindowSize() return h
end
Geyser.name = "Geyser Root Window"
Geyser.__index = Geyser

-- Create the window list for updates
Geyser.windowList = Geyser.windowList or {}
Geyser.windows = Geyser.windows or {}
Geyser.defer_updates = false

function Geyser:begin_update()
  self.defer_updates = true
end

function Geyser:end_update()
  self.defer_updates = false
  self:reposition()
end

function Geyser:reposition()
  if not self.defer_updates then
    GeyserReposition()
  end
end

--- Add a window to the list that this container manages.
-- this is the basis for the 2 add functions
-- @param window The window to add this container
function Geyser:base_add (window, cons)
  cons = cons or window -- 'cons' is optional

  -- Stop other container from controlling this window
  if window.container then
    window.container:remove(window)
  end

  -- Assume control of this window
  window.container = self
  -- Don't allow duplication of same name in container
  if not self.windowList[window.name] then
    self.windows[#self.windows+1] = window.name
  end
  self.windowList[window.name] = window

  window.windowname = window.windowname or window.container.windowname or "main"
  Geyser.set_constraints(window, cons, self)
  if not self.defer_updates then
    window:reposition()
  end
end

--- Add a window to the list that this container manages.
-- The window will be shown after added to the container
-- @param window The window to add this container
-- @param cons
-- @param passOn manages the inheritance of this add function
function Geyser:add (window, cons)
  self:base_add(window, cons)
  window:show()
end

--- Add a window to the list that this container manages. 
-- This add function prevents an element to be shown if it was hidden and hides an element if the 
-- container is hidden already
-- used by Adjustable.Container and changeContainer but can be used by any container using the new2 constructor
-- @param window The window to add this container
-- @param cons
-- @param passOn manages the inheritance of this add function. If set to true this add function will be inherited by all children.
-- @param exclude manages types which have to be excluded from overwriting their add function as they have their own
function Geyser:add2 (window, cons, passAdd2, exclude)
  cons = cons or window -- 'cons' is optional
  -- if the element doesn't want to use add2 use add instead
  if window.useAdd2 == false then
    Geyser.add(self, window, cons)
    return
  end
  -- don't overwrite these elements add function as they already use their own add
  exclude = exclude or {"hbox", "VBox", "adjustablecontainer"}

  if passAdd2 ~= false then
    passAdd2 = passAdd2 or self.useAdd2 or window.useAdd2 or false
  end

  self:base_add(window, cons)
  -- check all hidden values and hide if they are set
  if hidden then
    window:hide()
   end
  if auto_hidden or self.hidden or self.auto_hidden then
    window:hide(true)
  end
  -- if the hidden values are not set or false then show the window
  if not(window.hidden or window.auto_hidden) then
    window:show()
  end
  if passAdd2 then
    window.useAdd2 = true
    -- Don't overwrite hbox/vbox add functions as they have their own
    if window.add == Geyser.add and not (table.contains(exclude, window.type)) then
      window.add = window.add2
    end
  end
end

--- Removes a window from the list that it manages
-- @param window The window to remove from this container's
-- windowList
function Geyser:remove (window)
  self.windowList[window.name] = nil
  index = table.index_of(self.windows, window.name) or 0
  table.remove(self.windows, index)
end

local function changeNestContainer(windowname, label)
  for k,v in ipairs(label.nestedLabels) do
    if windowname ~= "main" then
      v:changeContainer(Geyser.windowList[windowname.."Container"].windowList[windowname])
    else
      v:changeContainer(Geyser)
    end
    if v.nestedLabels then
      changeNestContainer(windowname, v)
    end
  end
end

--- Removes a window from the parent it is in and puts it in a new one
-- This is only used internally.
-- @param window The new parents windowname
local function setMyWindow(self, windowname)
  windowname = windowname or "main"
  local name
  name = self.name
  if self.type == "mapper" then
    name = self.type
  end

  -- Change containerwindow for nested Labels
  if self.type == "label" and self.nestedLabels then
    changeNestContainer(windowname, self)
    closeAllLevels(self)
  end

  -- Prevent hidden children to get visible
  if self.hidden or self.auto_hidden then
    setWindow(windowname, name, 0, 0, false)
  else
    setWindow(windowname, name, 0, 0, true)
  end
end


--- Removes all containers windows from the parent they are in and puts them in a new one
-- This is only used internally
-- @param window The new parents windowname
local function setContainerWindow(self, windowname)
  self.windowname = windowname
  --Iterate through windows has a given order and prevents problems with z-coordinate
  for k,v in ipairs(self.windows) do
    setMyWindow(self.windowList[v], windowname)
    setContainerWindow(self.windowList[v], windowname)
  end
end

--- Change the container a window should be in
-- @param container The new container the window will be set in
function Geyser:changeContainer (container)
  --Change container to Geyser if "main" is given
  if type(container) == "string" and container:lower() == "main" then
    container = Geyser
  end
  --only a container has a windowList
  if not container or not container.windowList or self == container then
    return nil, "didn't get a valid container"
  end
  --Nothing to change
  if self.container == container then
    return nil, "nothing to change. "..self.name.." is already in this container"
  end
  --If there is no windowname then windowname is "main"
  local windowname = container.windowname
  windowname = windowname or "main"

  self.container:remove(self)
  if self.windowname ~= windowname then
    setMyWindow(self, windowname)
    setContainerWindow(self, windowname)
  end
  -- use add2 without overwriting childrens add functions
  container:add2(self, cons, false)
end
