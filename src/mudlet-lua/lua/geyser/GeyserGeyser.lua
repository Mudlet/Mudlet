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
Geyser = Geyser or {i = 0, x = 0, y = 0}

--Layout policies. Fixed = Do not stretch/shrink, Dynamic = Do stretch/shrink
Geyser.Fixed, Geyser.Dynamic = 0, 1

Geyser.width, Geyser.height = getMainWindowSize()

Geyser.get_x = function () return 0 end
Geyser.get_y = function () return 0 end
Geyser.get_width = function () return getMainWindowSize() end
Geyser.get_height = function () local w,h = getMainWindowSize() return h end
Geyser.name = "Geyser Root Window"
Geyser.__index = Geyser

-- Create the window list for updates
Geyser.windowList = Geyser.windowList or {}
Geyser.windows = Geyser.windows or {}

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
   Geyser.set_constraints(window, cons, self)
   window:reposition()
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
