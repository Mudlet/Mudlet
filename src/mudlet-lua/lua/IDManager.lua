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
    events = registerAnonymousEventHandler,
    triggers = tempTrigger,
    regexTriggers = tempRegexTrigger
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
    events = killAnonymousEventHandler,
    triggers = killTrigger,
    regexTriggers = killTrigger
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
  print("Deleting all " .. typ .. "...")
  if table.size(self[typ]) > 0 then
    for name,_ in pairs(self[typ]) do
      self:delete(name, typ)
    end
  end
  return true
end

function IDMgr:stopAllEvents()
  return self:stopAll("events")
end

function IDMgr:stopAllTimers()
  return self:stopAll("timers")
end

function IDMgr:stopAllTriggers()
  return self:stopAll("triggers")
end

function IDMgr:deleteAllEvents()
  return self:deleteAll("events")
end

function IDMgr:deleteAllTimers()
  return self:deleteAll("timers")
end

function IDMgr:deleteAllTriggers()
  local regex = self:deleteAll("regexTriggers")
  local substring = self:deleteAll("triggers")
  return regex and substring
end

function IDMgr:registerTimer(name, time, func, oneShot)
  local object = makeObject(time, func, oneShot or false)
  return self:register(name, "timers", object)
end

function IDMgr:registerEvent(name, event, func, oneShot)
  local object = makeObject(event, func, oneShot or false)
  return self:register(name, "events", object)
end

function IDMgr:registerTrigger(name, substring, func, expireAfter)
  local object = makeObject(substring, func, expireAfter or nil)
  return self:register(name, "triggers", object)
end

function IDMgr:registerRegexTrigger(name, substring, func, expireAfter)
  local object = makeObject(substring, func, expireAfter or nil)
  return self:register(name, "regexTriggers", object)
end

function IDMgr:stopTimer(name)
  return self:stop(name, "timers")
end

function IDMgr:stopEvent(name)
  return self:stop(name, "events")
end

function IDMgr:stopTrigger(name)
  if self:stop(name, "triggers") then return true end
  if self:stop(name, "regexTriggers") then return true end
  return false
end

function IDMgr:resumeTimer(name)
  return self:resume(name, "timers")
end

function IDMgr:resumeTrigger(name)
  if self:resume(name, "triggers") then return true end
  if self:resume(name, "regexTriggers") then return true end
  return false
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

function IDMgr:deleteTrigger(name)
  if self:delete(name, "triggers") then return true end
  if self:delete(name, "regexTriggers") then return true end
  return false
end

function IDMgr:emergencyStop()
  self:stopAll("events")
  self:stopAll("timers")
  self:stopAll("triggers")
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

function IDMgr:getTriggers()
  local triggerNames = table.update(table.keys(self.triggers), table.keys(self.regexTriggers))
  table.sort(triggerNames)
  return triggerNames
end

function IDMgr:new()
  local mgr = {
    events = {},
    timers = {},
    triggers = {},
    regexTriggers = {}
  }
  setmetatable(mgr, self)
  self.__index = self
  return mgr
end


-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getNewIDManager
-- give the user their own IDM to manage if that's what they want
function getNewIDManager()
  return IDMgr:new()
end

local idmanagers = {}

-- handles getting or making an IDM for the user
-- internal only, not documented
local function getManager(user)
  local mgr = idmanagers[user]
  if not mgr then
    mgr = IDMgr:new()
    idmanagers[user] = mgr
  end
  return mgr
end

-- internal only, used to format error messages
local function extractUpstreamError(funcName, err)
  local splitPattern = string.format("%s: ", funcName)
  local errMsg = err:split(splitPattern)[2]
  local argNumber = tonumber(errMsg:match("#(%d+)"))
  if argNumber then
    errMsg = errMsg:gsub("#" .. argNumber, "#" .. (argNumber + 2))
  end
  return errMsg
end

-- internal only, used to format error messages
local function userErrorMsg(funcName, userType)
  return string.format("%s: bad argument #1 type (user or package name as string expected, got %s!)", funcName, userType)
end

-- internal only, used to format error messages
local function nameErrorMsg(funcName, nameType)
  return string.format("%s: bad argument #2 type (handler name as string expected, got %s!)", funcName, nameType)
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#registerNamedEventHandler
function registerNamedEventHandler(user, name, eventName, handler, oneShot)
  local funcName = "registerNamedEventHandler"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local nameType = type(name)
  if nameType ~= "string" then
    printError(nameErrorMsg(funcName, nameType), true, true)
  end
  local mgr = getManager(user)
  local ok, err = mgr:registerEvent(name, eventName, handler, oneShot)
  if ok then
    return true
  end
  -- extract the error info from registerAnonymousEventHandler's error, increment argument number by 2
  -- to account for the user and name arguments, and then display it as our own error
  local errMsg = extractUpstreamError("registerAnonymousEventHandler", err)
  printError("registerNamedEventHandler: " .. errMsg, true, true)
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#stopNamedEventHandler
function stopNamedEventHandler(user, name)
  local funcName = "stopNamedEventHandler"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local nameType = type(name)
  if nameType ~= "string" then
    printError(nameErrorMsg(funcName, nameType), true, true)
  end
  local mgr = getManager(user)
  return mgr:stopEvent(name)
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resumeNamedEventHandler
function resumeNamedEventHandler(user,name)
  local funcName = "resumeNamedEventHandler"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local nameType = type(name)
  if nameType ~= "string" then
    printError(nameErrorMsg(funcName, nameType), true, true)
  end
  local mgr = getManager(user)
  return mgr:resumeEvent(name)
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteNamedEventHandler
function deleteNamedEventHandler(user,name)
  local funcName = "deleteNamedEventHandler"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local nameType = type(name)
  if nameType ~= "string" then
    printError(nameErrorMsg(funcName, nameType), true, true)
  end
  local mgr = getManager(user)
  return mgr:deleteEvent(name)
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getNamedEventHandlers
function getNamedEventHandlers(user)
  local funcName = "getNamedEventHandlers"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local mgr = getManager(user)
  return mgr:getEvents()
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#stopAllNamedEventHandlers
function stopAllNamedEventHandlers(user)
  local funcName = "stopAllNamedEventHandlers"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local mgr = getManager(user)
  return mgr:stopAllEvents()
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteAllNamedEventHandlers
function deleteAllNamedEventHandlers(user)
  local funcName = "deleteAllNamedEventHandlers"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local mgr = getManager(user)
  return mgr:deleteAllEvents()
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#registerNamedTimer
function registerNamedTimer(user,name, time, handler, oneShot)
  local funcName = "registerNamedTimer"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local nameType = type(name)
  if nameType ~= "string" then
    printError(nameErrorMsg(funcName, nameType), true, true)
  end
  local mgr = getManager(user)
  local ok, err = mgr:registerTimer(name, time, handler, oneShot)
  if ok then
    return true
  end
  -- extract the error info from tempTimer's error
  -- increment argument number by 1 (to account for the leading 'name' parameter)
  -- and then display it as our own error
  local errMsg = extractUpstreamError("tempTimer", err)
  printError("registerNamedTimer: " .. errMsg, true, true)
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#stopNamedTimer
function stopNamedTimer(user, name)
  local funcName = "stopNamedTimer"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local nameType = type(name)
  if nameType ~= "string" then
    printError(nameErrorMsg(funcName, nameType), true, true)
  end
  local mgr = getManager(user)
  return mgr:stopTimer(name)
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resumeNamedTimer
function resumeNamedTimer(user, name)
  local funcName = "resumeNamedTimer"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local nameType = type(name)
  if nameType ~= "string" then
    printError(nameErrorMsg(funcName, nameType), true, true)
  end
  local mgr = getManager(user)
  return mgr:resumeTimer(name)
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteNamedTimer
function deleteNamedTimer(user, name)
  local funcName = "deleteNamedTimer"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local nameType = type(name)
  if nameType ~= "string" then
    printError(nameErrorMsg(funcName, nameType), true, true)
  end
  local mgr = getManager(user)
  return mgr:deleteTimer(name)
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getNamedTimers
function getNamedTimers(user)
  local funcName = "getNamedTimers"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local mgr = getManager(user)
  return mgr:getTimers()
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#stopAllNamedTimers
function stopAllNamedTimers(user)
  local funcName = "stopAllNamedTimers"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local mgr = getManager(user)
  return mgr:stopAllTimers()
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteAllNamedTimers
function deleteAllNamedTimers(user)
  local funcName = "deleteAllNamedTimers"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local mgr = getManager(user)
  return mgr:deleteAllTimers()
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#registerNamedTrigger
function registerNamedTrigger(user, name, substring, handler, expireAfter)
  local funcName = "registerNamedTrigger"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local nameType = type(name)
  if nameType ~= "string" then
    printError(nameErrorMsg(funcName, nameType), true, true)
  end
  if type(substring) ~= "string" then
    printError(funcName .. ": bad argument #3 type (substring as string expected, got " .. type(substring) .. "!)", true, true)
  end
  if type(handler) ~= "function" then
    printError(funcName .. ": bad argument #4 type (function expected, got " .. type(handler) .. "!)", true, true)
  end
  local mgr = getManager(user)
  local ok, err = mgr:registerTrigger(name, substring, handler, expireAfter)
  if ok then
    return true
  end
  local errMsg = extractUpstreamError("tempTrigger", err)
  printError("registerNamedTrigger: " .. errMsg, true, true)
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#registerNamedRegexTrigger
function registerNamedRegexTrigger(user, name, substring, handler, expireAfter)
  local funcName = "registerNamedRegexTrigger"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local nameType = type(name)
  if nameType ~= "string" then
    printError(nameErrorMsg(funcName, nameType), true, true)
  end
  if type(substring) ~= "string" then
    printError(funcName .. ": bad argument #3 type (substring as string expected, got " .. type(substring) .. "!)", true, true)
  end
  if type(handler) ~= "function" then
    printError(funcName .. ": bad argument #4 type (function expected, got " .. type(handler) .. "!)", true, true)
  end
  local mgr = getManager(user)
  local ok, err = mgr:registerRegexTrigger(name, substring, handler, expireAfter)
  if ok then
    return true
  end
  local errMsg = extractUpstreamError("tempRegexTrigger", err)
  printError("registerNamedRegexTrigger: " .. errMsg, true, true)
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#stopNamedTrigger
function stopNamedTrigger(user, name)
  local funcName = "stopNamedTrigger"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local nameType = type(name)
  if nameType ~= "string" then
    printError(nameErrorMsg(funcName, nameType), true, true)
  end
  local mgr = getManager(user)
  return mgr:stopTrigger(name)
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#resumeNamedTrigger
function resumeNamedTrigger(user, name)
  local funcName = "resumeNamedTrigger"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local nameType = type(name)
  if nameType ~= "string" then
    printError(nameErrorMsg(funcName, nameType), true, true)
  end
  local mgr = getManager(user)
  return mgr:resumeTrigger(name)
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteNamedTrigger
function deleteNamedTrigger(user, name)
  local funcName = "deleteNamedTrigger"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local nameType = type(name)
  if nameType ~= "string" then
    printError(nameErrorMsg(funcName, nameType), true, true)
  end
  local mgr = getManager(user)
  return mgr:deleteTrigger(name)
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#getNamedTriggers
function getNamedTriggers(user)
  local funcName = "getNamedTriggers"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local mgr = getManager(user)
  return mgr:getTriggers()
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#stopAllNamedTriggers
function stopAllNamedTriggers(user)
  local funcName = "stopAllNamedTriggers"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local mgr = getManager(user)
  return mgr:stopAllTriggers()
end

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#deleteAllNamedTriggers
function deleteAllNamedTriggers(user)
  local funcName = "deleteAllNamedTriggers"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local mgr = getManager(user)
  return mgr:deleteAllTriggers()
end
