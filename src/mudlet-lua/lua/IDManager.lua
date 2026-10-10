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
  local register = reg[typ]
  local previous = self[typ][name]
  -- an event handler is killed before its replacement is made: one given as a function name would
  -- otherwise get the old handler's id back, and one made mid-dispatch would run in that dispatch
  local restore = false
  if typ == "events" and previous then
    restore = previous.handlerID ~= -1 and killAnonymousEventHandler(previous.handlerID) == true
    previous.handlerID = -1
  end
  local ok, id, refusal = pcall(register, object.trigger, object.func, object.oneShot)
  local err, refused
  if not ok then
    err = id
  elseif type(id) ~= "number" or id == -1 then
    -- refusals are answered, not raised: -1 from tempTimer for code that does not compile,
    -- nil from tempTrigger for an expiry below 1
    err, refused = refusal or "it could not be created", true
  end
  if err then
    if restore then
      local restored, oldID = pcall(register, previous.trigger, previous.func, previous.oneShot)
      if restored and type(oldID) == "number" and oldID ~= -1 then
        previous.handlerID = oldID
      end
    end
    return nil, err, refused
  end
  if typ ~= "events" then
    self:stop(name, typ)
  end
  object.handlerID = id
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
  -- named triggers live in two stores - registerNamedTrigger() fills "triggers"
  -- and registerNamedRegexTrigger() fills "regexTriggers" - so stopping them all
  -- has to walk both, exactly as deleteAllTriggers() does
  local regex = self:stopAll("regexTriggers")
  local substring = self:stopAll("triggers")
  return regex and substring
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
  self:stopAllTriggers()
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
  -- substring and regex names live in separate 1..n arrays, so merge them by
  -- value (table.n_union), not by index, or entries at the same index collide
  local triggerNames = table.n_union(table.keys(self.triggers), table.keys(self.regexTriggers))
  table.sort(triggerNames)
  return triggerNames
end

function IDMgr:remainingTime(name)
  local object = self.timers[name]
  if not object then
    return nil, "timer not found"
  end
  if object.handlerID == -1 then
    return nil, "timer is inactive"
  end
  local remaining = remainingTime(object.handlerID)
  if remaining == nil then
    -- underlying tempTimer is gone, e.g. an already-fired one-shot
    return nil, "timer is inactive"
  end
  return remaining
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
local function extractUpstreamError(funcName, err, refused)
  -- a refusal can quote the user's own code, so it is passed on as it is
  if refused then
    return err
  end
  local splitPattern = string.format("%s: ", funcName)
  local errMsg = err:split(splitPattern)[2]
  -- raised without the creator's name, e.g. a function name that does not compile: nothing to renumber
  if not errMsg then
    return err
  end
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
  local ok, err, refused = mgr:registerEvent(name, eventName, handler, oneShot)
  if ok then
    return true
  end
  -- extract the error info from registerAnonymousEventHandler's error, increment argument number by 2
  -- to account for the user and name arguments, and then display it as our own error
  local errMsg = extractUpstreamError("registerAnonymousEventHandler", err, refused)
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
  local ok, err, refused = mgr:registerTimer(name, time, handler, oneShot)
  if ok then
    return true
  end
  -- extract the error info from tempTimer's error
  -- increment argument number by 1 (to account for the leading 'name' parameter)
  -- and then display it as our own error
  local errMsg = extractUpstreamError("tempTimer", err, refused)
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

-- Documentation: https://wiki.mudlet.org/w/Manual:Lua_Functions#remainingNamedTimer
function remainingNamedTimer(user, name)
  local funcName = "remainingNamedTimer"
  local userType = type(user)
  if userType ~= "string" then
    printError(userErrorMsg(funcName, userType), true, true)
  end
  local nameType = type(name)
  if nameType ~= "string" then
    printError(nameErrorMsg(funcName, nameType), true, true)
  end
  local mgr = getManager(user)
  return mgr:remainingTime(name)
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
  local ok, err, refused = mgr:registerTrigger(name, substring, handler, expireAfter)
  if ok then
    return true
  end
  local errMsg = extractUpstreamError("tempTrigger", err, refused)
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
  local ok, err, refused = mgr:registerRegexTrigger(name, substring, handler, expireAfter)
  if ok then
    return true
  end
  local errMsg = extractUpstreamError("tempRegexTrigger", err, refused)
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
