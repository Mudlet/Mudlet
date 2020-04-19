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
-- automatic repositioning/resizing won't work with map widget
function Geyser.Mapper:reposition()
  if self.hidden or self.auto_hidden then
    return
  end
  if self.embedded then
    createMapper(self.windowname, self:get_x(), self:get_y(), self:get_width(), self:get_height())
  end
end

-- Overridden move and resize function - map widget does it differently right now
function Geyser.Mapper:move(x, y)
  if self.hidden or self.auto_hidden then
    return
  end
  Geyser.Container.move (self, x, y)
  if not self.embedded then
    moveMapWidget(self:get_x(), self:get_y())
  end
end

function Geyser.Mapper:resize(width, height)
  if self.hidden or self.auto_hidden then
    return
  end
  Geyser.Container.resize (self, width, height)
  if not self.embedded then
    resizeMapWidget(self:get_width(), self:get_height())
  end
end

function Geyser.Mapper:hide_impl()
  if self.embedded then
    createMapper(self.windowname, self:get_x(), self:get_y(), 0, 0)
  else
    closeMapWidget()
  end
end

function Geyser.Mapper:show_impl()
  if self.embedded then
    createMapper(self.windowname, self:get_x(), self:get_y(), self:get_width(), self:get_height())
  else
    openMapWidget()
  end
end

-- Overridden raise and lower functions
function Geyser.Mapper:raise()
	raiseWindow("mapper")
end

function Geyser.Mapper:lower()
	lowerWindow("mapper")
end

function Geyser.Mapper:setDockPosition(pos)
  if not self.embedded then
    return openMapWidget(pos)
  end
end

function Geyser.Mapper:setTitle(text)
  self.titleText = text
  return setMapWindowTitle(text)
end

function Geyser.Mapper:resetTitle()
  self.titleText = ""
  return resetMapWindowTitle()
end

-- Overridden constructor
function Geyser.Mapper:new (cons, container)
  cons = cons or {}
  cons.type = cons.type or "mapper"

  -- Call parent's constructor
  local me = self.parent:new(cons, container)
  me.windowname = me.windowname or me.container.windowname or "main"
  me.was_hidden = false

  -- Set the metatable.
  setmetatable(me, self)
  self.__index = self
  
  if me.embedded == nil and not me.dockPosition then
     me.embedded = true 
  end
  -----------------------------------------------------------
  -- Now create the Mapper using primitives
  if me.dockPosition and me.dockPosition:lower() == "floating" then
    me.dockPosition = "f"
  end
  if me.embedded then
    createMapper(me.windowname, me:get_x(), me:get_y(),
    me:get_width(), me:get_height())
  else
    me.embedded = false
    if me.dockPosition and me.dockPosition ~= "f" then
      openMapWidget(me.dockPosition)
    elseif me.dockPosition == "f" or cons.x or cons.y or cons.width or cons.height then 
      openMapWidget(me:get_x(), me:get_y(),
      me:get_width(), me:get_height())
    else
      openMapWidget()
    end

    if me.titleText then
      me:setTitle(me.titleText)
    else
      me:resetTitle()
    end
  end

  -- Set any defined colors
  Geyser.Color.applyColors(me)

  --print(" New in " .. self.name .. " : " .. me.name)
  return me
end
