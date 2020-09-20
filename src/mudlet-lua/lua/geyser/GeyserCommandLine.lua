--------------------------------------
-- --
-- The Geyser Layout Manager by guy --
--  CommandLine support by Edru     --
-- --
--------------------------------------

--- Represents a (sub)commandLine primitive
-- @class table
-- @name Geyser.CommandLine
Geyser.CommandLine = Geyser.Window:new({
  name = "CommandLineClass"
})

-- Save a reference to our parent constructor
Geyser.CommandLine.parent = Geyser.Window

--- Clears the cmdLine
-- see: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearCmdLine
function Geyser.CommandLine:clear()
  clearCmdLine(self.name)
end

--- prints text to the commandline and clears text if there was one previously
-- see: https://wiki.mudlet.org/w/Manual:Lua_Functions#printCmdLine(text)
function Geyser.CommandLine:print(text)
  printCmdLine(self.name, text)
end

--- appends text to the commandline
-- see: https://wiki.mudlet.org/w/Manual:Lua_Functions#appendCmdLine
function Geyser.CommandLine:append(text)
  appendCmdLine(self.name, text)
end

--- returns the text in the commandline
-- see: https://wiki.mudlet.org/w/Manual:Lua_Functions#getCmdLine
function Geyser.CommandLine:getText()
  return getCmdLine(self.name)
end

--- Sets an action to be used when text is send in this commandline. When this
-- function is called by the event system, text the commandline sends will be 
-- appended as the final argument (see @{sysCmdLineEvent}) and also in Geyser.Label
-- the setClickCallback events
-- @param func The function to use.
-- @param ... Parameters to pass to the function.
function Geyser.CommandLine:setAction(func, ...)
  setCmdLineAction(self.name, func, ...)
  self.actionFunc = func
  self.actionArgs = { ... }
end

--- Resets the action the command will be send to the game
function Geyser.CommandLine:resetAction()
  resetCmdLineAction(self.name)
  self.actionFunc = nil
  self.actionArgs = nil
end

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
