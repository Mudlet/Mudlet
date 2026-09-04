--- Represents a UserWindow Class.
-- <br/>See also: <a href="https://wiki.mudlet.org/w/Manual:Geyser#Geyser.UserWindow">Mudlet Manual</a>
-- @author guy
-- @author Edru
-- @module Geyser.UserWindow

--- Represents a UserWindow Class
-- Support of UserWindows in Geyser
-- UserWindows use also all the functions of MiniConsole as they contain one
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
--@param auto passed on by a container showing its children, and used by the
--            base class to pick which of the two hidden flags to clear
function Geyser.UserWindow:show(auto)
  self.Parent.show(self, auto)
end

--- Your UserWindow will be docked at position (pos):
-- works also if autoDock is disabled
--@param pos possible positions are "top", "bottom","left", "right" and "floating"
function Geyser.UserWindow:setDockPosition(pos)
  self.dockPosition = pos
  local docked = openUserWindow(self.name, false, self.autoDock, pos)
  -- opening the dock brings the window back on screen, so leaving self.hidden
  -- set would make the next hide() a no-op: Geyser.Container:hide() skips the
  -- widget for anything that already believes itself hidden. auto_hidden is
  -- deliberately left alone: clearing that one is the container cascade's job.
  if self.hidden then
    self:show()
  end
  return docked
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

--- Deletes the UserWindow, along with the root container Geyser made for it
function Geyser.UserWindow:type_delete()
  local root = self.rootContainer
  Geyser.MiniConsole.type_delete(self)
  -- Geyser.Container:new gives every user window a "<name>Container" root
  -- container. Left behind it is not inert: its get_width/get_height ask
  -- getUserWindowSize for a window that is gone, which answers with the main
  -- window's size, so the orphan claims all of it in every layout pass.
  -- Unregistering it rather than calling delete() on it keeps this safe when
  -- the user window is being deleted by that very container.
  if not root or not root.container then
    return
  end
  if table.is_empty(root.windowList) then
    root.container:remove(root)
  else
    -- anything else put in the root container by hand is still using it, so it
    -- has to stay - but it measures a user window that is about to be gone
    debugc(string.format(
      "Geyser.UserWindow: the root container of '%s' still holds other objects, so it is being left in place - it will report the main window's size from now on, because the user window it measured has been deleted",
      self.name))
  end
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
  elseif me.docked == false then
    -- this window is floated a few lines further down anyway, and docking it
    -- first takes the dock's size off the main window straight away - which is
    -- what the percentage constraints below would then be resolved against
    openUserWindow(me.name, me.restoreLayout, me.autoDock, "floating")
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

  -- Geyser.Container:new() settles the hidden constraint before there is a widget to hide, so the hide is made good here
  if me.hidden or me.auto_hidden then
    hideWindow(me.name)
  end

  Geyser.parentWindows = Geyser.parentWindows or {}
  Geyser.parentWindows[me.name] = me

  return me
end

--- Overridden constructor to use add2
function Geyser.UserWindow:new2 (cons)
  cons = cons or {}
  cons.useAdd2 = true
  local me = self:new(cons)
  return me
end
