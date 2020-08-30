--------------------------------------
-- --
-- The Geyser Layout Manager by guy --
--  CommandLine support by Edru     --
-- --
--------------------------------------

--- Represents a commandLine primitive
-- @class table
-- @name Geyser.CommandLine
-- @field wrapAt Where line wrapping occurs. Default is 300 characters.
Geyser.CommandLine = Geyser.Window:new({
  name = "CommandLineClass"
})


-- Save a reference to our parent constructor
Geyser.CommandLine.parent = Geyser.Window

-- Overridden constructor
function Geyser.CommandLine:new (cons, container)
  cons = cons or {}
  cons.type = cons.type or "commandLine"

  -- Call parent's constructor
  local me = self.parent:new(cons, container)
  me.windowname = me.windowname or me.container.windowname or "main"

  -- Set the metatable.
  setmetatable(me, self)
  self.__index = self

  createCommandLine(me.windowname, me.name, me:get_x(), me:get_y(), me:get_width(), me:get_height())
  return me
end

--- Overridden constructor to use add2
function Geyser.CommandLine:new2 (cons, container)
  cons = cons or {}
  cons.useAdd2 = true
  local me = self:new(cons, container)
  return me
end
