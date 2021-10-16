local IDMgr = {}
local function makeObject(trigger, func, oneShot)
  local object = {
    trigger = trigger,
    func    = func,
    oneShot = oneShot,
  }
  return object
end

-- internal function, not documented
function IDMgr:register(name, typ, object)
  local reg = {
    timers = tempTimer,
    events = registerAnonymousEventHandler
  }
  self:stop(name, typ)
  local trigger, func, oneShot = object.trigger, object.func, object.oneShot
  local register = reg[typ]
  local ok, err = pcall(register, trigger, func, oneShot)
  if not ok then
    return nil, err
  end
  object.handlerID = err
  self[typ][name] = object
  return true
end

-- internal function, not documented
function IDMgr:stop(name, typ)
  local killfuncs = {
    timers = killTimer,
    events = killAnonymousEventHandler
  }
  local object = self[typ][name]
  if not object then
    return false
  end
  local kill = killfuncs[typ]
  kill(object.handlerID)
  object.handlerID = -1
  return true
end

-- internal function, not documented
function IDMgr:resume(name, typ)
  local object = self[typ][name]
  if not object then
    return false
  end
  return self:register(name, typ, object)
end

-- internal function, not documented
function IDMgr:stopAll(typ)
  for name,_ in pairs(self[typ]) do
    self:stop(name, typ)
  end
  return true
end

-- internal function, not documented
function IDMgr:delete(name, typ)
  local object = self[typ][name]
  if not object then
    return false
  end
  self:stop(name, typ)
  self[typ][name] = nil
  return true
end

-- internal function, not documented
function IDMgr:deleteAll(typ)
  for name,_ in pairs(self[typ]) do
    self:delete(name, typ)
  end
  return true
end

function IDMgr:stopAllEvents()
  return self:stopAll("events")
end

function IDMgr:stopAllTimers()
  return self:stopAll("timers")
end

function IDMgr:deleteAllEvents()
  return self:deleteAll("events")
end

function IDMgr:deleteAllTimers()
  return self:deleteAll("timers")
end

function IDMgr:registerTimer(name, time, func, oneShot)
  local object = makeObject(time, func, oneShot or false)
  return self:register(name, "timers", object)
end

function IDMgr:registerEvent(name, event, func, oneShot)
  local object = makeObject(event, func, oneShot or false)
  return self:register(name, "events", object)
end

function IDMgr:stopTimer(name)
  return self:stop(name, "timers")
end

function IDMgr:stopEvent(name)
  return self:stop(name, "events")
end

function IDMgr:resumeTimer(name)
  return self:resume(name, "timers")
end

function IDMgr:resumeEvent(name)
  return self:resume(name, "events")
end

function IDMgr:deleteTimer(name)
  return self:delete(name, "timers")
end

function IDMgr:deleteEvent(name)
  return self:delete(name, "events")
end

function IDMgr:emergencyStop()
  self:stopAll("events")
  self:stopAll("timers")
  return true
end

function IDMgr:getEvents()
  local eventNames = table.keys(self.events)
  table.sort(eventNames)
  return eventNames
end

function IDMgr:getTimers()
  local timerNames = table.keys(self.timers)
  table.sort(timerNames)
  return timerNames
end

function IDMgr:new()
  local mgr = {
    events = {},
    timers = {}
  }
  setmetatable(mgr, self)
  self.__index = self
  return mgr
end

local mgr = IDMgr:new()

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getNewIDManager
function getNewIDManager()
  return IDMgr:new()
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#registerNamedEventHandler
function registerNamedEventHandler(name, eventName, handler, oneShot)
  local ok, err = mgr:registerEvent(name, eventName, handler, oneShot)
  if ok then
    return true
  end
  local errMsg = err:split("registerAnonymousEventHandler: ")[2]
  local argNumber = tonumber(errMsg:match("#(%d+)"))
  if argNumber then
    errMsg = errMsg:gsub("#" .. argNumber, "#" .. (argNumber + 1))
  end
  printError("registerNamedEventHandler: " .. errMsg, true, true)
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#stopNamedEventHandler
function stopNamedEventHandler(name)
  local nameType = type(name)
  if nameType ~= "string" then
    return nil, "name as string expected, got " .. nameType
  end
  return mgr:stopEvent(name)
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resumeNamedEventHandler
function resumeNamedEventHandler(name)
  local nameType = type(name)
  if nameType ~= "string" then
    return nil, "name as string expected, got " .. nameType
  end
  return mgr:resumeEvent(name)
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteNamedEventHandler
function deleteNamedEventHandler(name)
  local nameType = type(name)
  if nameType ~= "string" then
    return nil, "name as string expected, got " .. nameType
  end
  return mgr:deleteEvent(name)
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getNamedEventHandlers
function getNamedEventHandlers()
  return mgr:getEvents()
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#stopAllNamedEventHandlers
function stopAllNamedEventHandlers()
  return mgr:stopAllEvents()
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteAllNamedEventHandlers
function deleteAllNamedEventHandlers()
  return mgr:deleteAllEvents()
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#registerNamedTimer
function registerNamedTimer(name, time, handler, oneShot)
  local ok, err = mgr:registerTimer(name, time, handler, oneShot)
  if ok then
    return true
  end
  local errMsg = err:split("tempTimer: ")[2]
  local argNumber = tonumber(err:match("#(%d+)"))
  if argNumber then
    errMsg = errMsg:gsub("#" .. argNumber, "#" .. (argNumber + 1))
  end
  printError("registerNamedTimer: " .. errMsg, true, true)
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#stopNamedTimer
function stopNamedTimer(name)
  local nameType = type(name)
  if nameType ~= "string" then
    return nil, "name as string expected, got " .. nameType
  end
  return mgr:stopTimer(name)
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resumeNamedTimer
function resumeNamedTimer(name)
  local nameType = type(name)
  if nameType ~= "string" then
    return nil, "name as string expected, got " .. nameType
  end
  return mgr:resumeTimer(name)
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteNamedTimer
function deleteNamedTimer(name)
  local nameType = type(name)
  if nameType ~= "string" then
    return nil, "name as string expected, got " .. nameType
  end
  return mgr:deleteTimer(name)
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getNamedTimers
function getNamedTimers()
  return mgr:getTimers()
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#stopAllNamedTimers
function stopAllNamedTimers()
  return mgr:stopAllTimers()
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteAllNamedTimers
function deleteAllNamedTimers()
  return mgr:deleteAllTimers()
end