--------------------------------------
-- --
-- The Geyser Layout Manager by guy --
-- createMapper support by Vadi --
-- --
--------------------------------------

--- Represents a mapper primitive
-- @class table
-- @name Geyser.Mapper
-- @field wrapAt Where line wrapping occurs. Default is 300 characters.
Geyser.Mapper = Geyser.Window:new({
      name = "MapperClass"
})


-- Save a reference to our parent constructor
Geyser.Mapper.parent = Geyser.Window

-- Overridden reposition function - mapper does it differently right now
function Geyser.Mapper:reposition()
   if self.hidden then return end
   createMapper(self:get_x(), self:get_y(), self:get_width(), self:get_height())
end

function Geyser.Mapper:hide_impl()
   createMapper(self:get_x(), self:get_y(), 0, 0)
end

function Geyser.Mapper:show_impl()
   createMapper(self:get_x(), self:get_y(), self:get_width(), self:get_height())
end

-- Overridden constructor
function Geyser.Mapper:new (cons, container)
   cons = cons or {}
   cons.type = cons.type or "mapper"

   -- Call parent's constructor
   local me = self.parent:new(cons, container)
   
   me.was_hidden = false
   
   -- Set the metatable.
   setmetatable(me, self)
   self.__index = self

   -----------------------------------------------------------
   -- Now create the Mapper using primitives
   createMapper(me:get_x(), me:get_y(),
                     me:get_width(), me:get_height())

   -- Set any defined colors
   Geyser.Color.applyColors(me)

   --print(" New in " .. self.name .. " : " .. me.name)
   return me
end