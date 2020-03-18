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
-- @param window The window to add this container
function Geyser:add (window, cons)
  cons = cons or window -- 'cons' is optional

  -- Stop other container from controlling this window
  if window.container then
    window.container:remove(window)
  end

  -- Assume control of this window
  window.container = self
  self.windowList[window.name] = window
  table.insert(self.windows, window.name)
  window.windowname = window.windowname or window.container.windowname or "main"
  Geyser.set_constraints(window, cons, self)
  if not self.defer_updates then
    window:reposition()
  end
  window:show()
end

--- Removes a window from the list that it manages
-- @param window The window to remove from this container's
-- windowList
function Geyser:remove (window)
  self.windowList[window.name] = nil
  index = table.index_of(self.windows, window.name) or 0
  table.remove(self.windows, index)
end


--- Removes a window from the parent it is in and put's it in a new one
-- This is used internally, don't use it. Use changeContainer instead
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
    for k,v in ipairs(self.nestedLabels) do
      if windowname ~= "main" then
        v:changeContainer(Geyser.windowList[windowname.."Container"].windowList[windowname])
      else
        v:changeContainer(Geyser)
      end
    end
    closeAllLevels(self)
  end
  
  -- Prevent hidden children to get visible  
  if self.hidden or self.auto_hidden then
    setWindow(windowname, name, 0, 0, 0)
  else 
    setWindow(windowname, name, 0, 0, 1)
  end
end


--- Removes all containers windows from the parent they are in and put's them in a new one
-- This is used internally, don't use it. Use changeContainer instead
-- @param window The new parents windowname 
local function setContainerWindow(self, windowname)
  self.windowname = windowname
  for k,v in pairs(self.windowList) do
    setMyWindow(v, windowname)
    setContainerWindow(v, windowname)
  end
end

--- Removes a window from the container that it manages
-- @param container The new container the window will be set in
function Geyser:changeContainer (container)
  --Nothing to change
  if self.container == container then
    return
  end 
  --If there is no windowname then windowname is main
  local windowname = container.windowname
  windowname = windowname or "main"

  self.container:remove(self)
  if self.windowname ~= container.windowname then
    setMyWindow(self, container.windowname)
    setContainerWindow(self, container.windowname)
  end
  container:add(self)
end
