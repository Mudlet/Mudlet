--------------------------------------
-- The Geyser Layout Manager by guy --
--------------------------------------

--- Represents a video_player primitive
-- @class table
-- @name Geyser.VideoPlayer
-- @field wrapAt Where line wrapping occurs. Default is 300 characters.
Geyser.VideoPlayer = Geyser.Window:new({
  name = "VideoPlayerClass"
})


-- Save a reference to our parent constructor
Geyser.VideoPlayer.parent = Geyser.Window

-- Overridden reposition function - mapper does it differently right now
-- automatic repositioning/resizing won't work with map widget
function Geyser.VideoPlayer:reposition()
  if self.hidden or self.auto_hidden then
    return
  end
  if self.embedded then
    createVideoPlayer(self.windowname, self:get_x(), self:get_y(), self:get_width(), self:get_height())
  end
end

-- Overridden move and resize function - map widget does it differently right now
function Geyser.VideoPlayer:move(x, y)
  if self.hidden or self.auto_hidden then
    return
  end
  Geyser.Container.move (self, x, y)
  if not self.embedded then
    moveVideoPlayerWidget(self:get_x(), self:get_y())
  end
end

function Geyser.VideoPlayer:resize(width, height)
  if self.hidden or self.auto_hidden then
    return
  end
  Geyser.Container.resize (self, width, height)
  if not self.embedded then
    resizeVideoPlayerWidget(self:get_width(), self:get_height())
  end
end

function Geyser.VideoPlayer:hide_impl()
  if self.embedded then
    createVideoPlayer(self.windowname, self:get_x(), self:get_y(), 0, 0)
  else
    closeVideoPlayerWidget()
  end
end

function Geyser.VideoPlayer:show_impl()
  if self.embedded then
    createVideoPlayer(self.windowname, self:get_x(), self:get_y(), self:get_width(), self:get_height())
  else
    openVideoPlayerWidget()
  end
end

-- Overridden raise and lower functions
function Geyser.VideoPlayer:raise()
	raiseWindow("video_player")
end

function Geyser.VideoPlayer:lower()
	lowerWindow("video_player")
end

function Geyser.VideoPlayer:setDockPosition(pos)
  if not self.embedded then
    return openVideoPlayerWidget(pos)
  end
end

function Geyser.VideoPlayer:setTitle(text)
  self.titleText = text
  return setVideoPlayerWindowTitle(text)
end

function Geyser.VideoPlayer:resetTitle()
  self.titleText = ""
  return resetVideoPlayerWindowTitle()
end

-- Overridden constructor
function Geyser.VideoPlayer:new (cons, container)
  cons = cons or {}
  cons.type = cons.type or "video_player"

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
  -- Now create the VideoPlayer using primitives
  if me.dockPosition and me.dockPosition:lower() == "floating" then
    me.dockPosition = "f"
  end
  if me.embedded then
    createVideoPlayer(me.windowname, me:get_x(), me:get_y(),
    me:get_width(), me:get_height())
  else
    me.embedded = false
    if me.dockPosition and me.dockPosition ~= "f" then
      openVideoPlayerWidget(me.dockPosition)
    elseif me.dockPosition == "f" or cons.x or cons.y or cons.width or cons.height then 
      openVideoPlayerWidget(me:get_x(), me:get_y(),
      me:get_width(), me:get_height())
    else
      openVideoPlayerWidget()
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

--- Overridden constructor to use add2
function Geyser.VideoPlayer:new2 (cons, container)
  cons = cons or {}
  cons.useAdd2 = true
  local me = self:new(cons, container)
  return me
end
