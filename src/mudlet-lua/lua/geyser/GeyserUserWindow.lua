--------------------------------------
--                                  --
-- The Geyser Layout Manager by guy --
-- UserWindow support by Edru       --
--                                  --
--------------------------------------

--- Represents a UserWindow Class
--Support of UserWindows in Geyser
--UserWindows use also all the functions of MiniConsole as they contain one
-- @class table
-- @name Geyser.UserWindow
Geyser.UserWindow = Geyser.MiniConsole:new({
  name = "UserWindowClass",
  color = "black"})

--Set Constraints and use main Window (Geyser) as reference
function Geyser.UserWindow:set_uwconstr()
  Geyser.set_constraints(self,self,Geyser)
end
--- Moves the UserWindow:
-- is set to floating state if this function is used
--@param x x-coordinate
--@param y y-coordinate
function Geyser.UserWindow:move(x,y)
  self.x = x or self.x
  self.y = y or self.y
  self:set_uwconstr()
  moveWindow(self.name,self.get_x(),self.get_y())
  self:resetWindow()
end

--- Resizes the UserWindow:
-- only works if your UserWindow is in floating state
--@param width userwindow widh
--@param height userwindow height
function Geyser.UserWindow:resize(width,height)
  self.width = width or self.width
  self.height = height or self.height
  self:set_uwconstr()
  resizeWindow(self.name,self.get_width(),self.get_height())
  self:resetWindow()
end

--Reset Root Container to cover all the UserWindow
function Geyser.UserWindow:resetWindow()
  self.x = 0
  self.y = 0
  self.width = "100%"
  self.height = "100%"
  self:set_constraints(self)
end

--Override show to keep the dimensions of the UserWindow
function Geyser.UserWindow:show()
  local w,h = self.get_width(),self.get_height()
  self.Parent.show(self)
  self:resize(w,h)
end

--- Your UserWindow will be docked at position (pos):
-- works also if autoDock is disabled
--@param pos possible positions are "top", "bottom","left", "right" and "floating"
function Geyser.UserWindow:setDockPosition(pos)
  self.dockPosition = pos
  return openUserWindow(self.name, false, self.autoDock, pos)
end

--- Enables the automatic docking at the borders if it was previously disabled
function Geyser.UserWindow:enableAutoDock()
  self.autoDock = true
  return openUserWindow(self.name, self.restoreLayout, true)
end

--- Changes the title text of your UserWindow
--@param text title text of your UserWindow
function Geyser.UserWindow:setTitle(text)
  self.titleText = text
  return setUserWindowTitle(self.name, text)
end

--- Disables the automatic docking at the borders
function Geyser.UserWindow:disableAutoDock()
  self.autoDock = false
  return openUserWindow(self.name, self.restoreLayout, false)
end

--- Resets the title of your UserWindow to default
function Geyser.UserWindow:resetTitle()
  self.titleText = ""
  return resetUserWindowTitle(self.name)
end

--- Sets the style sheet of the UserWindows (border and title area)
-- @param css The style sheet string
function Geyser.UserWindow:setStyleSheet(css)
  css = css or self.stylesheet
  setUserWindowStyleSheet(self.name, css)
  self.stylesheet = css
end

Geyser.UserWindow.Parent = Geyser.Window

--- Geyser UserWindow constructor
---@param cons table of Geyser MiniConsole options such as name, width, and height.
-- UserWindow parameters are:
-- @param[opt='false'] cons.restoreLayout restore previously saved Layout or not (is set to false if not set)
-- @param[opt='nil'] cons.docked start the container in a docked state (if set to true x, y, width and height will be ignored)
-- @param[opt='true'] cons.autoDock dock at the borders if the window is put at them (set to true if not set)
-- @param[opt='right'] cons.dockPosition choose the dockPosition at creation. Possible values are: "top", "bottom", "right", "left" and "floating" (is set to "right" if not set)
-- @param cons.titleText choose your UserWindows title (set to default title if not set)
function Geyser.UserWindow:new(cons)
  cons = cons or {}
  cons.name = cons.name or Geyser.nameGen()
  cons.type = cons.type or "userwindow"
  cons.windowname = cons.name
  cons.width = cons.width or 435
  cons.height = cons.height or 375
  cons.x = cons.x or 10
  cons.y = cons.y or 140
  --Root Container for UserWindows
  local me = self.Parent:new(cons)
  -- Set the metatable.
  setmetatable(me, self)
  self.__index = self

  me.restoreLayout = me.restoreLayout or false
  me.docked = me.docked or false
  if not (me.autoDock == false) then
    me.autoDock = true
  end
  me.dockPosition = me.dockPosition or "r"

  if me.restoreLayout then
    openUserWindow(me.name, me.restoreLayout, me.autoDock)
  else
    openUserWindow(me.name, me.restoreLayout, me.autoDock, me.dockPosition)
  end

  -- Set any defined colors
  Geyser.Color.applyColors(me)
  
  if cons.fontSize then
    me:setFontSize(cons.fontSize)
  else
    me:setFontSize(8)
    cons.fontSize = 8
  end
  if cons.scrollBar then
    me:enableScrollBar()
  else
    me:disableScrollBar()
  end
  if cons.font then
    me:setFont(cons.font)
  end
  if cons.wrapAt == "auto" then
    me:enableAutoWrap()
  elseif cons.wrapAt then
    me:setWrap(cons.wrapAt)
  end
  
  --Resizing not possible if docked
  --Docking position not choosable if restoreLayout don't move/resize at start
  if me.docked == false and me.restoreLayout == false then
    me.dockPosition = "floating"
    me:move(cons.x, cons.y)
    me:resize(cons.width, cons.height)
  end

  if me.titleText then
    me:setTitle(me.titleText)
  else
    me:resetTitle()
  end
  
  me:resetWindow()
  if me.stylesheet then me:setStyleSheet() end

  return me
end

--- Overridden constructor to use add2
function Geyser.UserWindow:new2 (cons)
  cons = cons or {}
  cons.useAdd2 = true
  local me = self:new(cons)
  return me
end