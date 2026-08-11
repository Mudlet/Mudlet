--- Represents a mapper primitive.
-- <br/>See also: <a href="https://wiki.mudlet.org/w/Manual:Geyser#Geyser.Mapper">Mudlet Manual</a>
-- @author guy
-- @author Vadi
-- @module Geyser.Mapper

--- Represents a mapper primitive.
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
    -- A title only reaches a map window that is on screen, so one this mapper
    -- set while it was hidden has to be applied now instead. Only one that did
    -- not land: an unconditional apply here would overwrite a title set through
    -- setMapWindowTitle() directly every time any mapper is shown. An empty
    -- titleText is a reset rather than "no title", which is why this tracks
    -- whether the call failed instead of whether the text is empty.
    if self.titlePending then
      self.titlePending = false
      setMapWindowTitle(self.titleText)
    end
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
  local applied, message = setMapWindowTitle(text)
  self.titlePending = not applied
  return applied, message
end

function Geyser.Mapper:resetTitle()
  self.titleText = ""
  -- resetMapWindowTitle() is setMapWindowTitle(""), so show_impl applying
  -- titleText covers a reset as well
  local applied, message = resetMapWindowTitle()
  self.titlePending = not applied
  return applied, message
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
-- This only has an effect if add2 is being used as for the standard add method me.hidden and me.auto_hidden is always false at creation/initialisation
  if me.hidden or me.auto_hidden then
    me:hide_impl()
  end
  -- Set any defined colors
  Geyser.Color.applyColors(me)

  --print(" New in " .. self.name .. " : " .. me.name)
  return me
end

--- Deletes the mapper - note that mappers are typically managed differently
-- This hides the mapper but does not destroy the underlying map data
function Geyser.Mapper:type_delete()
  -- Mappers use closeMapWidget to be hidden, but there's no true "delete"
  -- function for mappers as they're typically singleton per profile
  closeMapWidget(self.windowname)
end

--- Overridden constructor to use add2
function Geyser.Mapper:new2 (cons, container)
  cons = cons or {}
  cons.useAdd2 = true
  local me = self:new(cons, container)
  return me
end
