--------------------------------------
--                                  --
-- The Geyser Layout Manager by guy --
--                                  --
--------------------------------------

--- Represents a generic container with positional information.
-- Has no notion of contents and is used to contain other windows
-- and impose some sense of order.
-- @class table
-- @name Geyser.Container
-- @field parent The parent class of this window
-- @field type The type of this window, usually lowercase of the classname and
--             can be used in checks for certain types. For a Container
--             instance, type is “container.”
-- @field name The name of this window. This is the same name that Mudlet will
--             use internally for primitive types like labels and
--             miniconsoles.  If not specified in the constraints table during
--             creation, an anonymous name unique to this session will be
--             made up.
-- @field x The x-coordinate relative to this window's container, not absolute
--          from the Mudlet main window. This is not a number, but a
--          constraint specification. To find out the numeric x-coordinate in
--          terms of pixels from the Mudlet main window's origin, use the
--          get_x() method. Default is "10px".
-- @field y The y-coordinate relative to this window's container, not absolute
--          from the Mudlet main window. This is not a number, but a
--          constraint specification. To find out the numeric y-coordinate in
--          terms of pixels from the Mudlet main window's origin, use the
--          get_y() method. Default is "10px".
-- @field width The width of this window, possibly relative to the window's
--              container. This is not a number, but a constraint
--              specification. To find out the numeric width in terms of
--              pixels, use the get_width() method. Default is "300px".
-- @field height The height of this window, possibly relative to the window's
--               container. This is not a number, but a constraint
--               specification. To find out the numeric width in terms of
--               pixels, use the get_width() method. Default is "200px".
-- @field windowList This is a list of all windows held by this container. It
--                   used to handle proper resizing of held windows as well as
--                   during show()s and hide()s to recursive show and hide all
--                   children windows.
-- @field fontSize The default size font used when calculating dimensions
--                 based on the character constraint. Default is 8.
Geyser.Container =  {
   name = "ContainerClass",
   x = "10px",
   y = "10px",
   height = "200px",
   width = "300px",
   windowList = {},
   fontSize = 8,
}

--- Responsible for placing/moving/resizing this window to the correct place/size.
-- Called on window resize events.
function Geyser.Container:reposition ()
   local x,y,w,h = self:get_x(), self:get_y(), self:get_width(), self:get_height()
   moveWindow(self.name, self:get_x(), self:get_y())
   resizeWindow(self.name, self:get_width(), self:get_height())
   
   -- deal with all children of this container
   for k,v in pairs(self.windowList) do
      if k ~= self then
         v:reposition()
      end
   end

   -- Calls optional redraw method if it is available to cause a gui element to
   -- redraw itself after moving.
   if self.redraw then self:redraw() end
end

--- Hides this window and all its contained windows.
function Geyser.Container:hide ()
   hideWindow(self.name)
   for _,v in pairs(self.windowList) do
      v:hide()
   end
end

--- Shows this window and all windows it contains.
function Geyser.Container:show ()
   showWindow(self.name)
   for _,v in pairs(self.windowList) do
      v:show()
   end
end

--- Moves this window according to the new x and y contraints set. 
-- @param x New x constraint to use. If nil, uses current value.
-- @param y New y constraint to use. If nil, uses current value.
function Geyser.Container:move (x, y)
   self.x = x or self.x
   self.y = y or self.y
   self:set_constraints(self)
end

--- Resizes this window according to the new width and height constraints set. 
-- @param width New width constraint to use.  If nil, uses current value.
-- @param height New height constraint to use.  If nil, uses current value.
function Geyser.Container:resize (width, height)
   self.width = width or self.width
   self.height = height or self.height
   self:set_constraints(self)
end

--- Sets the default font size for this window.
-- Will resizes this window if necessary to meet constraints.
-- @param fontSize The new font size to use.
function Geyser.Container:setFontSize (fontSize)
   if type(fontSize) ~= "number" then
      error("fontSize must be a number")
      return
   end
   self.fontSize = fontSize or self.fontSize
   self:set_constraints()
end

--- Sets all contraints (x, y, width, height) for this window.
-- @param cons Any Lua table that contains appropriate constraint entries.
function Geyser.Container:set_constraints (cons)
   cons = cons or self
   Geyser.set_constraints(self, cons, self.container)
   for k,v in pairs(self.windowList) do
      v:set_constraints(v)
   end
end

--- Flashes a white box over the dimensions of this container. 
-- This is very useful to see where a container actually is if you've 
-- forgotten its details. 
-- @param time Time in seconds to flash for, default is 1.0s.
function Geyser.Container:flash (time)
   local time = time or 1.0
   local x, y, width, height = self.get_x(), self.get_y(), self.get_width(), self.get_height()
   local name = self.name .. "_dimensions_flash"
   createLabel(name, x, y, width, height, 1)
   resizeWindow(name, width, height)
   moveWindow(name, x, y)
   setBackgroundColor(name, 190, 190, 190, 128)
   showWindow(name)
   tempTimer(time, "hideWindow(\"" .. name .. "\")")
end

Geyser.Container.parent = Geyser.Container -- I'm my own grandpa too!

-- Someone has to be the root!
setmetatable(Geyser.Container, Geyser)

--- Constructor for containers.
-- This function creates a new container/window
-- @param cons Any Lua table that contains appropriate constraint entries.
--             Include any parameter such as name or fontSize in cons
--             that are to be used for the new window.
function Geyser.Container:new(cons, container)
   -- create new table for the container and copy over constraints
   local me = Geyser.copyTable(cons)

   -- enforce a default type, name and parent
   me.type = me.type or "container"
   me.name = me.name or Geyser.nameGen()
   me.windowList = {}

   -- Set the metatable.
   setmetatable(me, self)
   self.__index = self

   -- If we're not not a class definition then add to a controlling
   -- container.
   if not string.find(me.name, ".*Class") then
      -- If passed in a container, add me to that container
      if container then
         container:add(me)
      else
         -- Else assume the root window is my container
         Geyser:add(me)
      end
   end

   --print("New in " .. self.name .. " : " .. me.name)
   return me
end

