--------------------------------------
--                                  --
-- The Geyser Layout Manager by guy --
--                                  --
--------------------------------------

--- Represents a UserWindow Class
-- @class table
-- @name Geyser.UserWindow
--UserWindow is just a MiniConsole
Geyser.UserWindow = Geyser.MiniConsole:new({
  name = "UserWindowClass"})

--Set Constraints and use main Window (Geyser) as reference
function Geyser.UserWindow:set_uwconstr()
  Geyser.set_constraints(self,self,Geyser)
end

function Geyser.UserWindow:move(x,y)
  self.x = x or self.x
  self.y = y or self.y
  self:set_uwconstr()
  moveWindow(self.name,self.get_x(),self.get_y())
  self:resetWindow()
end

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

Geyser.UserWindow.Parent = Geyser.Window

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
  me = self.Parent:new(cons)
  -- Set the metatable.
  setmetatable(me, self)
  self.__index = self
  
  me.restoreLayout = me.restoreLayout or false
  me.docked = me.docked or false

  openUserWindow(me.name,me.restoreLayout)

  --Resizing not possible if docked
  --Docking position not choosable
  if me.docked == false then
      me:move(cons.x,cons.y)
      me:resize(cons.width,cons.height)
  end

  me:resetWindow()
  return me
end
